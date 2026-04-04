"""
trader.py

Fully-automated AI trading system driven by two debating AI agents:

  • Analyst AI     — reads technical indicators and builds a trade thesis
  • Risk Manager AI — stress-tests the thesis, sets position size and stop-loss

The agents reach a consensus (BUY / SELL / HOLD) and the system either:
  (a) runs a paper simulation  — default, no broker credentials needed
  (b) executes live orders via Alpaca paper/live trading API

Usage
─────
  # Paper simulation (no keys needed):
  python trader.py --symbols AAPL MSFT TSLA --cash 10000

  # With OpenAI for live AI debate:
  export OPENAI_API_KEY="sk-..."
  python trader.py --symbols AAPL TSLA --cash 10000 --rounds 2

  # Alpaca paper brokerage (free account at alpaca.markets):
  export ALPACA_API_KEY="..."
  export ALPACA_SECRET_KEY="..."
  export ALPACA_BASE_URL="https://paper-api.alpaca.markets"
  python trader.py --symbols AAPL MSFT --broker alpaca

  # Live trading (real money — use with extreme caution):
  export ALPACA_BASE_URL="https://api.alpaca.markets"
  python trader.py --symbols AAPL --broker alpaca --live

WARNING: This software is for educational purposes. Past performance does not
guarantee future results. Never invest money you cannot afford to lose.
"""

from __future__ import annotations

import argparse
import os
import sys
import textwrap
import time
from dataclasses import dataclass, field
from datetime import datetime, timezone
from typing import Optional

# ---------------------------------------------------------------------------
# Optional imports
# ---------------------------------------------------------------------------
try:
    import pandas as pd
    _PANDAS_AVAILABLE = True
except ImportError:
    _PANDAS_AVAILABLE = False

try:
    import yfinance as yf
    _YF_AVAILABLE = True
except ImportError:
    _YF_AVAILABLE = False

try:
    import ta
    _TA_AVAILABLE = True
except ImportError:
    _TA_AVAILABLE = False

try:
    from openai import OpenAI
    _OPENAI_AVAILABLE = True
except ImportError:
    _OPENAI_AVAILABLE = False

try:
    import alpaca_trade_api as tradeapi
    _ALPACA_AVAILABLE = True
except ImportError:
    _ALPACA_AVAILABLE = False

# ---------------------------------------------------------------------------
# Risk / position-sizing constants
# ---------------------------------------------------------------------------

MAX_POSITION_PCT         = 0.25   # largest fraction of cash in a single position
KELLY_SCALE_FACTOR       = 0.30   # scales confidence → position size (Kelly-inspired)
MAX_STOP_LOSS_PCT        = 0.08   # hard cap on stop-loss distance
ATR_MULTIPLIER           = 1.5    # stop-loss = entry - ATR_MULTIPLIER × ATR
REWARD_RISK_RATIO        = 2.0    # take-profit = stop-loss distance × this ratio
MAX_PORTFOLIO_ALLOCATION = 0.80   # cap total new buy allocation at 80% of cash

# ---------------------------------------------------------------------------
# ANSI colours
# ---------------------------------------------------------------------------
COLORS = {
    "analyst":  "\033[94m",   # blue
    "risk":     "\033[93m",   # yellow
    "system":   "\033[96m",   # cyan
    "buy":      "\033[92m",   # green
    "sell":     "\033[91m",   # red
    "hold":     "\033[37m",   # white
    "reset":    "\033[0m",
    "bold":     "\033[1m",
    "dim":      "\033[2m",
}

AGENT_NAMES = {
    "analyst": "Analyst AI",
    "risk":    "Risk Manager AI",
}


def _c(role: str, text: str) -> str:
    color = COLORS.get(role, "")
    return f"{COLORS['bold']}{color}{text}{COLORS['reset']}"


def _stream(text: str, delay: float = 0.008) -> None:
    for ch in text:
        sys.stdout.write(ch)
        sys.stdout.flush()
        time.sleep(delay)
    print()


def _header(title: str) -> None:
    bar = "═" * 70
    print(_c("system", f"\n{bar}"))
    print(_c("system", f"  {title}"))
    print(_c("system", bar))


# ---------------------------------------------------------------------------
# Market data & indicators
# ---------------------------------------------------------------------------

@dataclass
class MarketSnapshot:
    symbol:       str
    current:      float
    change_pct:   float
    sma_20:       float
    sma_50:       float
    rsi_14:       float
    macd:         float
    macd_signal:  float
    bb_upper:     float
    bb_lower:     float
    volume_ratio: float   # today vs 20-day avg
    atr_14:       float   # Average True Range — volatility proxy


def fetch_snapshot(symbol: str) -> Optional[MarketSnapshot]:
    """Download recent OHLCV data and compute technical indicators."""
    if not (_PANDAS_AVAILABLE and _YF_AVAILABLE):
        return None
    try:
        ticker = yf.Ticker(symbol)
        df = ticker.history(period="3mo", interval="1d", auto_adjust=True)
        if df is None or len(df) < 52:
            return None

        close = df["Close"]
        high  = df["High"]
        low   = df["Low"]
        vol   = df["Volume"]

        current    = float(close.iloc[-1])
        prev_close = float(close.iloc[-2])
        change_pct = (current - prev_close) / prev_close * 100

        sma_20 = float(close.rolling(20).mean().iloc[-1])
        sma_50 = float(close.rolling(50).mean().iloc[-1])

        if _TA_AVAILABLE:
            rsi_14      = float(ta.momentum.RSIIndicator(close, window=14).rsi().iloc[-1])
            macd_obj    = ta.trend.MACD(close)
            macd        = float(macd_obj.macd().iloc[-1])
            macd_signal = float(macd_obj.macd_signal().iloc[-1])
            bb_obj      = ta.volatility.BollingerBands(close, window=20)
            bb_upper    = float(bb_obj.bollinger_hband().iloc[-1])
            bb_lower    = float(bb_obj.bollinger_lband().iloc[-1])
            atr_14      = float(ta.volatility.AverageTrueRange(high, low, close, window=14).average_true_range().iloc[-1])
        else:
            # Fallback manual calculations
            delta  = close.diff()
            gain   = delta.clip(lower=0).rolling(14).mean()
            loss   = (-delta.clip(upper=0)).rolling(14).mean()
            rs     = gain / loss.replace(0, float("nan"))
            rsi_14 = float(100 - 100 / (1 + rs.iloc[-1]))

            ema12       = close.ewm(span=12, adjust=False).mean()
            ema26       = close.ewm(span=26, adjust=False).mean()
            macd_series = ema12 - ema26
            macd        = float(macd_series.iloc[-1])
            macd_signal = float(macd_series.ewm(span=9, adjust=False).mean().iloc[-1])

            rolling_std = close.rolling(20).std()
            bb_upper    = float(sma_20 + 2 * rolling_std.iloc[-1])
            bb_lower    = float(sma_20 - 2 * rolling_std.iloc[-1])

            tr = pd.concat([
                high - low,
                (high - close.shift()).abs(),
                (low  - close.shift()).abs(),
            ], axis=1).max(axis=1)
            atr_14 = float(tr.rolling(14).mean().iloc[-1])

        vol_ratio = float(vol.iloc[-1] / vol.rolling(20).mean().iloc[-1])

        return MarketSnapshot(
            symbol=symbol,
            current=current,
            change_pct=change_pct,
            sma_20=sma_20,
            sma_50=sma_50,
            rsi_14=rsi_14,
            macd=macd,
            macd_signal=macd_signal,
            bb_upper=bb_upper,
            bb_lower=bb_lower,
            volume_ratio=vol_ratio,
            atr_14=atr_14,
        )
    except Exception as exc:  # noqa: BLE001
        print(f"{COLORS['dim']}[Warning: could not fetch {symbol}: {exc}]{COLORS['reset']}")
        return None


def _snapshot_summary(s: MarketSnapshot) -> str:
    return (
        f"Symbol: {s.symbol}\n"
        f"Price:  ${s.current:.2f}  (day change: {s.change_pct:+.2f}%)\n"
        f"SMA-20: ${s.sma_20:.2f}  SMA-50: ${s.sma_50:.2f}\n"
        f"RSI-14: {s.rsi_14:.1f}  MACD: {s.macd:.4f}  Signal: {s.macd_signal:.4f}\n"
        f"BB upper: ${s.bb_upper:.2f}  lower: ${s.bb_lower:.2f}\n"
        f"Volume ratio (vs 20d avg): {s.volume_ratio:.2f}x\n"
        f"ATR-14 (volatility): ${s.atr_14:.2f}"
    )


# ---------------------------------------------------------------------------
# Rule-based signal generator (used when no API key is present)
# ---------------------------------------------------------------------------

@dataclass
class TradeSignal:
    action:         str          # "BUY" | "SELL" | "HOLD"
    confidence:     float        # 0–1
    position_pct:   float        # fraction of available cash to deploy
    stop_loss_pct:  float        # e.g. 0.05 = 5% below entry
    take_profit_pct: float       # e.g. 0.10 = 10% above entry
    analyst_notes:  str = ""
    risk_notes:     str = ""


def rule_based_signal(s: MarketSnapshot) -> TradeSignal:
    """
    Generate a trade signal using classical technical analysis rules.
    Used as a fallback when no OpenAI key is set.
    """
    bullish = 0
    bearish = 0

    # Trend
    if s.current > s.sma_20 > s.sma_50:
        bullish += 2
    elif s.current < s.sma_20 < s.sma_50:
        bearish += 2

    # RSI
    if s.rsi_14 < 30:
        bullish += 2    # oversold
    elif s.rsi_14 > 70:
        bearish += 2    # overbought
    elif 40 < s.rsi_14 < 60:
        bullish += 1

    # MACD crossover
    if s.macd > s.macd_signal:
        bullish += 1
    else:
        bearish += 1

    # Bollinger Bands
    if s.current <= s.bb_lower:
        bullish += 1    # price touched lower band
    elif s.current >= s.bb_upper:
        bearish += 1    # price touched upper band

    # Volume confirmation
    if s.volume_ratio > 1.5:
        if bullish > bearish:
            bullish += 1
        else:
            bearish += 1

    total = bullish + bearish or 1
    bull_pct = bullish / total

    if bull_pct >= 0.6:
        action = "BUY"
        confidence = bull_pct
    elif bull_pct <= 0.4:
        action = "SELL"
        confidence = 1 - bull_pct
    else:
        action = "HOLD"
        confidence = 0.5

    # Position sizing: Kelly-inspired — scale with confidence, cap at MAX_POSITION_PCT
    position_pct = min(MAX_POSITION_PCT, confidence * KELLY_SCALE_FACTOR) if action != "HOLD" else 0.0

    # Risk management: stop-loss = ATR_MULTIPLIER × ATR below entry
    stop_loss_pct   = min(MAX_STOP_LOSS_PCT, (s.atr_14 / s.current) * ATR_MULTIPLIER)
    take_profit_pct = stop_loss_pct * REWARD_RISK_RATIO

    analyst_notes = (
        f"RSI={s.rsi_14:.1f}, MACD {'above' if s.macd > s.macd_signal else 'below'} signal, "
        f"price {'above' if s.current > s.sma_20 else 'below'} SMA-20, "
        f"volume {s.volume_ratio:.1f}x average. Bullish signals: {bullish}, bearish: {bearish}."
    )
    risk_notes = (
        f"ATR-based stop at -{stop_loss_pct*100:.1f}%, "
        f"take-profit at +{take_profit_pct*100:.1f}%. "
        f"Deploying {position_pct*100:.1f}% of cash."
    )

    return TradeSignal(
        action=action,
        confidence=confidence,
        position_pct=position_pct,
        stop_loss_pct=stop_loss_pct,
        take_profit_pct=take_profit_pct,
        analyst_notes=analyst_notes,
        risk_notes=risk_notes,
    )


# ---------------------------------------------------------------------------
# AI-powered debate agents
# ---------------------------------------------------------------------------

ANALYST_SYSTEM = textwrap.dedent("""\
    You are Analyst AI, an expert quantitative trader specialising in technical
    analysis. Given a market data snapshot you will:
    1. Identify the dominant trend and momentum signals.
    2. Highlight key support/resistance levels from the Bollinger Bands and SMAs.
    3. State a clear trade thesis: BUY, SELL, or HOLD, with a confidence score
       between 0 and 1.
    4. Suggest a position size as a percentage of available cash (max 25%).
    Keep your response to 4–6 sentences and be specific about the numbers.
    End with a proposed stop-loss and take-profit level.
""")

RISK_SYSTEM = textwrap.dedent("""\
    You are Risk Manager AI, a disciplined risk officer focused on capital
    preservation. You receive the Analyst's thesis and the same market data.
    Your job is to:
    1. Stress-test the trade thesis — identify what could go wrong.
    2. Evaluate whether the reward-to-risk ratio is acceptable (target >= 2:1).
    3. Adjust the position size downward if volatility is elevated.
    4. Either APPROVE, REJECT, or MODIFY the trade, stating your final verdict
       clearly in the last sentence.
    Keep your response to 4–6 sentences. Be direct and cite specific indicator
    values to justify your decision.
""")


class TradingAgent:
    """An AI agent backed by the OpenAI Chat Completions API."""

    def __init__(self, role: str, system_prompt: str, client: "OpenAI", model: str):
        self.role          = role
        self.system_prompt = system_prompt
        self.client        = client
        self.model         = model
        self.history: list[dict] = []

    def reply(self, prompt: str) -> str:
        self.history.append({"role": "user", "content": prompt})
        messages = [{"role": "system", "content": self.system_prompt}] + self.history
        response = self.client.chat.completions.create(
            model=self.model,
            messages=messages,
            temperature=0.5,
            max_tokens=400,
        )
        answer = response.choices[0].message.content.strip()
        self.history.append({"role": "assistant", "content": answer})
        return answer


def _parse_action_from_text(text: str) -> str:
    upper = text.upper()
    for word in ("BUY", "SELL", "HOLD"):
        if word in upper:
            return word
    return "HOLD"


def ai_debate_signal(
    snapshot: MarketSnapshot,
    client: "OpenAI",
    model: str,
    rounds: int = 2,
) -> TradeSignal:
    """
    Run a multi-round debate between Analyst AI and Risk Manager AI to produce
    a consensus trade signal.
    """
    analyst = TradingAgent("analyst", ANALYST_SYSTEM, client, model)
    risk    = TradingAgent("risk",    RISK_SYSTEM,    client, model)

    summary = _snapshot_summary(snapshot)

    # Round 1: Analyst builds thesis
    analyst_prompt = (
        f"Here is the current market data for {snapshot.symbol}:\n\n{summary}\n\n"
        f"Please provide your trade thesis."
    )
    analyst_reply = analyst.reply(analyst_prompt)
    _print_agent("analyst", analyst_reply)

    # Subsequent rounds: Risk pushes back, Analyst may revise
    risk_msg = analyst_reply
    for _ in range(rounds - 1):
        risk_reply = risk.reply(
            f"Market data:\n{summary}\n\nAnalyst thesis:\n{risk_msg}\n\nYour risk assessment:"
        )
        _print_agent("risk", risk_reply)

        analyst_msg = analyst.reply(
            f"Risk Manager feedback:\n{risk_reply}\n\n"
            f"Do you maintain or revise your thesis? State final BUY/SELL/HOLD."
        )
        _print_agent("analyst", analyst_msg)
        risk_msg = analyst_msg

    # Final risk verdict
    final_risk = risk.reply(
        f"Final analyst position:\n{risk_msg}\n\n"
        f"Give your final APPROVE/REJECT/MODIFY verdict with adjusted position sizing."
    )
    _print_agent("risk", final_risk)

    # Parse consensus
    combined  = risk_msg + " " + final_risk
    action    = _parse_action_from_text(combined)
    if "REJECT" in final_risk.upper():
        action = "HOLD"

    # Fallback to rule-based sizing
    rb = rule_based_signal(snapshot)
    position_pct = rb.position_pct if action != "HOLD" else 0.0

    return TradeSignal(
        action=action,
        confidence=0.7,
        position_pct=position_pct,
        stop_loss_pct=rb.stop_loss_pct,
        take_profit_pct=rb.take_profit_pct,
        analyst_notes=analyst_reply,
        risk_notes=final_risk,
    )


def _print_agent(role: str, text: str) -> None:
    print(_c(role, f"\n── {AGENT_NAMES[role]} ──"))
    _stream(textwrap.fill(text, width=88))


# ---------------------------------------------------------------------------
# Paper simulation portfolio
# ---------------------------------------------------------------------------

@dataclass
class Position:
    symbol:       str
    shares:       float
    entry_price:  float
    stop_loss:    float
    take_profit:  float


@dataclass
class Portfolio:
    cash:       float
    positions:  dict[str, Position] = field(default_factory=dict)
    trade_log:  list[dict]          = field(default_factory=list)

    # ── helpers ──────────────────────────────────────────────────────────────

    def market_value(self, prices: dict[str, float]) -> float:
        mv = self.cash
        for sym, pos in self.positions.items():
            mv += pos.shares * prices.get(sym, pos.entry_price)
        return mv

    def execute(self, signal: TradeSignal, snapshot: MarketSnapshot) -> None:
        sym   = snapshot.symbol
        price = snapshot.current
        now   = datetime.now(tz=timezone.utc).isoformat()

        if signal.action == "BUY" and sym not in self.positions:
            spend  = self.cash * signal.position_pct
            shares = spend / price
            if shares < 0.0001 or spend > self.cash:
                return
            self.cash -= spend
            self.positions[sym] = Position(
                symbol=sym,
                shares=shares,
                entry_price=price,
                stop_loss=price * (1 - signal.stop_loss_pct),
                take_profit=price * (1 + signal.take_profit_pct),
            )
            self.trade_log.append({
                "time": now, "symbol": sym, "action": "BUY",
                "shares": round(shares, 4), "price": price,
                "stop_loss": round(price * (1 - signal.stop_loss_pct), 2),
                "take_profit": round(price * (1 + signal.take_profit_pct), 2),
            })
            self._print_trade("buy", sym, "BUY", shares, price, signal)

        elif signal.action == "SELL" and sym in self.positions:
            pos   = self.positions.pop(sym)
            proceeds = pos.shares * price
            self.cash += proceeds
            pnl = proceeds - pos.shares * pos.entry_price
            self.trade_log.append({
                "time": now, "symbol": sym, "action": "SELL",
                "shares": round(pos.shares, 4), "price": price,
                "pnl": round(pnl, 2),
            })
            self._print_trade("sell", sym, "SELL", pos.shares, price, signal, pnl=pnl)

    def check_stops(self, prices: dict[str, float]) -> None:
        """Automatically exit positions that hit stop-loss or take-profit."""
        to_close: list[tuple[str, float, str]] = []
        for sym, pos in self.positions.items():
            price = prices.get(sym, pos.entry_price)
            if price <= pos.stop_loss:
                to_close.append((sym, price, "STOP-LOSS"))
            elif price >= pos.take_profit:
                to_close.append((sym, price, "TAKE-PROFIT"))

        for sym, price, reason in to_close:
            pos = self.positions.pop(sym)
            proceeds = pos.shares * price
            self.cash += proceeds
            pnl = proceeds - pos.shares * pos.entry_price
            now = datetime.now(tz=timezone.utc).isoformat()
            self.trade_log.append({
                "time": now, "symbol": sym, "action": f"CLOSE ({reason})",
                "shares": round(pos.shares, 4), "price": price, "pnl": round(pnl, 2),
            })
            color = "buy" if pnl >= 0 else "sell"
            print(_c(color,
                f"  ✓ AUTO-CLOSE {sym} @ ${price:.2f}  [{reason}]  "
                f"P&L: ${pnl:+.2f}"
            ))

    @staticmethod
    def _print_trade(
        color: str,
        sym: str,
        action: str,
        shares: float,
        price: float,
        signal: TradeSignal,
        pnl: float | None = None,
    ) -> None:
        pnl_str = f"  P&L: ${pnl:+.2f}" if pnl is not None else ""
        print(_c(color,
            f"\n  ▶ {action} {shares:.4f} × {sym} @ ${price:.2f} "
            f"(confidence {signal.confidence:.0%}){pnl_str}"
        ))

    def print_summary(self, prices: dict[str, float], initial_cash: float) -> None:
        mv = self.market_value(prices)
        pnl = mv - initial_cash
        pnl_color = "buy" if pnl >= 0 else "sell"
        _header("Portfolio Summary")
        print(f"  Cash:           ${self.cash:>12,.2f}")
        for sym, pos in self.positions.items():
            cur   = prices.get(sym, pos.entry_price)
            unrealised = (cur - pos.entry_price) * pos.shares
            print(
                f"  {sym:<8} {pos.shares:.4f} shares @ ${pos.entry_price:.2f}  "
                f"now ${cur:.2f}  unrealised: ${unrealised:+.2f}"
            )
        print(f"  {'─'*50}")
        print(_c(pnl_color, f"  Total value:    ${mv:>12,.2f}"))
        print(_c(pnl_color, f"  Total P&L:      ${pnl:>+12,.2f}  ({pnl/initial_cash*100:+.2f}%)"))
        if self.trade_log:
            print(f"\n  Trade history ({len(self.trade_log)} orders):")
            for t in self.trade_log:
                pnl_part = f"  P&L ${t['pnl']:+.2f}" if "pnl" in t else ""
                print(f"    {t['time'][:19]}  {t['action']:<22} {t['symbol']:<8} "
                      f"{t.get('shares',0):.4f} @ ${t.get('price',0):.2f}{pnl_part}")
        print()


# ---------------------------------------------------------------------------
# Alpaca broker integration
# ---------------------------------------------------------------------------

class AlpacaBroker:
    """Thin wrapper around the Alpaca REST API for live / paper order execution."""

    def __init__(self, api_key: str, secret_key: str, base_url: str):
        if not _ALPACA_AVAILABLE:
            raise RuntimeError(
                "alpaca-trade-api is not installed. "
                "Run: pip install alpaca-trade-api"
            )
        self.api = tradeapi.REST(api_key, secret_key, base_url, api_version="v2")

    def get_account(self) -> dict:
        acct = self.api.get_account()
        return {"cash": float(acct.cash), "equity": float(acct.equity)}

    def submit_order(
        self,
        symbol: str,
        qty: float,
        side: str,
        stop_loss: float | None = None,
        take_profit: float | None = None,
    ) -> dict:
        order_kwargs: dict = {
            "symbol":     symbol,
            "qty":        round(qty, 2),
            "side":       side.lower(),
            "type":       "market",
            "time_in_force": "day",
        }
        if stop_loss and take_profit:
            order_kwargs["order_class"] = "bracket"
            order_kwargs["stop_loss"]   = {"stop_price": round(stop_loss,  2)}
            order_kwargs["take_profit"] = {"limit_price": round(take_profit, 2)}

        order = self.api.submit_order(**order_kwargs)
        return {
            "id":     order.id,
            "symbol": order.symbol,
            "qty":    order.qty,
            "side":   order.side,
            "status": order.status,
        }

    def get_positions(self) -> list[dict]:
        return [
            {
                "symbol": p.symbol,
                "qty":    float(p.qty),
                "avg_entry": float(p.avg_entry_price),
                "current":   float(p.current_price),
                "unrealised_pnl": float(p.unrealized_pl),
            }
            for p in self.api.list_positions()
        ]


def execute_alpaca(
    broker: AlpacaBroker,
    signal: TradeSignal,
    snapshot: MarketSnapshot,
    live: bool,
) -> None:
    """Execute a trade via Alpaca, logging clearly."""
    if signal.action == "HOLD":
        print(_c("hold", f"  HOLD {snapshot.symbol} — no order placed."))
        return

    acct  = broker.get_account()
    cash  = acct["cash"]
    spend = cash * signal.position_pct
    mode  = "LIVE" if live else "PAPER"

    if signal.action == "BUY":
        shares = spend / snapshot.current
        if shares < 0.001:
            print(_c("hold", f"  Insufficient cash for {snapshot.symbol}."))
            return
        stop  = snapshot.current * (1 - signal.stop_loss_pct)
        tp    = snapshot.current * (1 + signal.take_profit_pct)
        print(_c("buy",
            f"  ▶ [{mode}] BUY {shares:.2f} × {snapshot.symbol} "
            f"@ market  stop=${stop:.2f}  tp=${tp:.2f}"
        ))
        result = broker.submit_order(snapshot.symbol, shares, "buy", stop, tp)
        print(_c("system", f"     Order ID: {result['id']}  status: {result['status']}"))

    elif signal.action == "SELL":
        positions = {p["symbol"]: p for p in broker.get_positions()}
        if snapshot.symbol not in positions:
            print(_c("hold", f"  No open position in {snapshot.symbol} to sell."))
            return
        pos = positions[snapshot.symbol]
        print(_c("sell",
            f"  ▶ [{mode}] SELL {pos['qty']:.2f} × {snapshot.symbol} @ market"
        ))
        result = broker.submit_order(snapshot.symbol, pos["qty"], "sell")
        print(_c("system", f"     Order ID: {result['id']}  status: {result['status']}"))


# ---------------------------------------------------------------------------
# Investment planning session
# ---------------------------------------------------------------------------

def plan_investment_schedule(
    symbols: list[str],
    cash: float,
    signals: dict[str, TradeSignal],
    snapshots: dict[str, MarketSnapshot],
) -> None:
    """
    Print a structured investment plan covering:
    - Which assets to buy/sell/hold and why
    - Recommended entry timing (immediate vs. wait for pullback)
    - Dollar allocation per asset
    - Combined portfolio risk estimate
    """
    _header("Investment Plan")

    buys  = {s: sig for s, sig in signals.items() if sig.action == "BUY"}
    sells = {s: sig for s, sig in signals.items() if sig.action == "SELL"}
    holds = {s: sig for s, sig in signals.items() if sig.action == "HOLD"}

    total_alloc = sum(sig.position_pct for sig in buys.values())
    # Normalise if over-allocated
    if total_alloc > MAX_PORTFOLIO_ALLOCATION:
        scale = MAX_PORTFOLIO_ALLOCATION / total_alloc
        for sig in buys.values():
            sig.position_pct *= scale

    print(f"\n  Available cash: ${cash:,.2f}\n")

    if buys:
        print(_c("buy", "  ── BUY ORDERS ──"))
        for sym, sig in buys.items():
            snap   = snapshots.get(sym)
            dollar = cash * sig.position_pct
            shares = dollar / snap.current if snap else 0
            timing = (
                "Immediate — price is above key moving averages with strong momentum."
                if snap and snap.current > snap.sma_20
                else "Consider waiting for a retest of the SMA-20 for a better entry."
            )
            print(
                f"    {sym:<8} buy ~{shares:.2f} shares @ ${snap.current:.2f}  "
                f"(${dollar:,.0f}, {sig.position_pct*100:.1f}% of cash)\n"
                f"            stop=${snap.current*(1-sig.stop_loss_pct):.2f}  "
                f"tp=${snap.current*(1+sig.take_profit_pct):.2f}  "
                f"confidence={sig.confidence:.0%}\n"
                f"            Timing: {timing}"
            )

    if sells:
        print(_c("sell", "\n  ── SELL ORDERS ──"))
        for sym, sig in sells.items():
            snap = snapshots.get(sym)
            print(
                f"    {sym:<8} close position @ ${snap.current:.2f}  "
                f"confidence={sig.confidence:.0%}"
            )

    if holds:
        print(_c("hold", "\n  ── HOLD (no action) ──"))
        for sym in holds:
            print(f"    {sym}")

    # Portfolio-level risk estimate
    weighted_stop = sum(
        sig.stop_loss_pct * sig.position_pct
        for sig in buys.values()
    )
    max_drawdown = weighted_stop * cash
    print(f"\n  Estimated max drawdown on new positions: ${max_drawdown:,.2f} "
          f"({weighted_stop*100:.2f}% of cash)\n")


# ---------------------------------------------------------------------------
# Main orchestration
# ---------------------------------------------------------------------------

def run_trader(
    symbols: list[str],
    initial_cash: float,
    model: str,
    debate_rounds: int,
    broker_mode: str,
    live: bool,
    api_key: str,
) -> None:
    use_ai = bool(api_key) and _OPENAI_AVAILABLE
    client = OpenAI(api_key=api_key) if use_ai else None

    _header(f"Pablo Automated Trader  |  {'LIVE' if live else 'PAPER'} mode")
    if not use_ai:
        print(_c("system", "  [Rule-based mode — set OPENAI_API_KEY for AI debate]"))

    # ── Fetch market data ─────────────────────────────────────────────────
    print(_c("system", "\n  Fetching market data …"))
    snapshots: dict[str, MarketSnapshot] = {}
    for sym in symbols:
        snap = fetch_snapshot(sym)
        if snap:
            snapshots[sym] = snap
            print(f"  {sym}: ${snap.current:.2f}  "
                  f"RSI={snap.rsi_14:.1f}  SMA20={snap.sma_20:.2f}")
        else:
            print(_c("dim", f"  {sym}: data unavailable — skipping"))

    if not snapshots:
        print(_c("sell", "\n  No market data available. "
                  "Ensure yfinance/pandas are installed and you have internet access."))
        return

    # ── Generate signals ──────────────────────────────────────────────────
    signals: dict[str, TradeSignal] = {}
    for sym, snap in snapshots.items():
        _header(f"Analysing {sym}")
        if use_ai:
            signals[sym] = ai_debate_signal(snap, client, model, debate_rounds)
        else:
            sig = rule_based_signal(snap)
            signals[sym] = sig
            color = sig.action.lower() if sig.action in ("BUY", "SELL") else "hold"
            print(_c(color, f"\n  Signal: {sig.action}  (confidence {sig.confidence:.0%})"))
            print(f"  Analyst:      {sig.analyst_notes}")
            print(f"  Risk Manager: {sig.risk_notes}")

    # ── Investment plan ───────────────────────────────────────────────────
    plan_investment_schedule(symbols, initial_cash, signals, snapshots)

    # ── Execute trades ────────────────────────────────────────────────────
    _header("Order Execution")

    if broker_mode == "alpaca":
        ak  = os.environ.get("ALPACA_API_KEY",   "")
        sk  = os.environ.get("ALPACA_SECRET_KEY", "")
        url = os.environ.get("ALPACA_BASE_URL",   "https://paper-api.alpaca.markets")
        if not (ak and sk):
            print(_c("sell", "  ALPACA_API_KEY / ALPACA_SECRET_KEY not set — aborting."))
            return
        if not _ALPACA_AVAILABLE:
            print(_c("sell", "  alpaca-trade-api not installed — run: pip install alpaca-trade-api"))
            return
        broker = AlpacaBroker(ak, sk, url)
        for sym, sig in signals.items():
            execute_alpaca(broker, sig, snapshots[sym], live)

    else:
        # Paper simulation
        portfolio = Portfolio(cash=initial_cash)
        prices    = {sym: snap.current for sym, snap in snapshots.items()}

        portfolio.check_stops(prices)   # close stale positions first
        for sym, sig in signals.items():
            portfolio.execute(sig, snapshots[sym])

        portfolio.print_summary(prices, initial_cash)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description=(
            "Pablo Automated Trader — AI-driven investment planning and trade execution. "
            "Defaults to paper simulation (no real money)."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent("""\
            Examples:
              python trader.py --symbols AAPL MSFT TSLA
              python trader.py --symbols NVDA --cash 50000 --rounds 3
              python trader.py --symbols AAPL --broker alpaca
        """),
    )
    parser.add_argument(
        "--symbols", nargs="+", default=["AAPL", "MSFT", "TSLA"],
        help="Stock ticker symbols to analyse (default: AAPL MSFT TSLA)",
    )
    parser.add_argument(
        "--cash", type=float, default=10_000.0,
        help="Starting cash for paper simulation (default: 10000)",
    )
    parser.add_argument(
        "--model", default="gpt-4o-mini",
        help="OpenAI model for AI debate (default: gpt-4o-mini)",
    )
    parser.add_argument(
        "--rounds", type=int, default=2,
        help="Number of AI debate rounds per symbol (default: 2)",
    )
    parser.add_argument(
        "--broker", choices=["paper", "alpaca"], default="paper",
        help="Execution backend (default: paper)",
    )
    parser.add_argument(
        "--live", action="store_true",
        help="Enable live trading via Alpaca (DANGER: uses real money)",
    )
    args = parser.parse_args()

    if args.live and args.broker != "alpaca":
        parser.error("--live requires --broker alpaca")

    if args.live:
        print(_c("sell", "\n  ⚠  LIVE TRADING MODE — real money will be used!"))
        confirm = input("  Type 'YES I UNDERSTAND' to continue: ")
        if confirm.strip() != "YES I UNDERSTAND":
            print("  Aborted.")
            return

    if not (_PANDAS_AVAILABLE and _YF_AVAILABLE):
        missing = []
        if not _PANDAS_AVAILABLE:
            missing.append("pandas")
        if not _YF_AVAILABLE:
            missing.append("yfinance")
        print(_c("sell", f"\n  Missing required packages: {', '.join(missing)}"))
        print("  Install with: pip install " + " ".join(missing))
        return

    api_key = os.environ.get("OPENAI_API_KEY", "")

    run_trader(
        symbols=args.symbols,
        initial_cash=args.cash,
        model=args.model,
        debate_rounds=args.rounds,
        broker_mode=args.broker,
        live=args.live,
        api_key=api_key,
    )


if __name__ == "__main__":
    main()
