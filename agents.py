"""
agents.py

Dynamic AI Agent Factory for Pablo.

Lets you define, register, spawn, and run any number of focused AI agents,
each with its own persona, tools, and task loop.

Built-in agent catalogue
────────────────────────
  news_analyst        — scans headlines and rates market-moving stories
  earnings_summariser — digests earnings-call transcripts / reports
  crypto_watcher      — tracks crypto market conditions & sentiment
  portfolio_monitor   — audits a portfolio for concentration/risk issues
  sentiment_scanner   — scores social-media / news sentiment per ticker
  options_flow        — flags unusual options activity patterns
  quant_researcher    — deep-dives into quantitative strategies
  macro_economist     — tracks macroeconomic indicators (CPI, GDP, rates)

You can also define your own agents via YAML (see --spec / --define) or by
calling AgentFactory.register() in Python.

Usage
─────
  # List all available agents:
  python agents.py --list

  # Run one built-in agent in demo (no API key) mode:
  python agents.py --run news_analyst

  # Run with a live OpenAI key:
  export OPENAI_API_KEY="sk-..."
  python agents.py --run earnings_summariser --input "AAPL Q1 2024 earnings beat EPS by 0.12"

  # Run multiple agents in sequence:
  python agents.py --run news_analyst sentiment_scanner --input "Fed raises rates by 25bp"

  # Spawn a fully custom agent from a YAML spec file:
  python agents.py --spec my_agent.yaml --input "..."

  # Define a new agent interactively and add it to the registry:
  python agents.py --define

  # Run two agents as a debate pair (like quantum_chat / trader):
  python agents.py --debate news_analyst sentiment_scanner \
      --input "Tesla misses delivery targets"
"""

from __future__ import annotations

import argparse
import os
import sys
import textwrap
import time
from dataclasses import dataclass, field
from typing import Any, Optional

# ---------------------------------------------------------------------------
# Optional imports
# ---------------------------------------------------------------------------
try:
    import yaml as _yaml
    _YAML_AVAILABLE = True
except ImportError:
    _YAML_AVAILABLE = False

try:
    from openai import OpenAI
    _OPENAI_AVAILABLE = True
except ImportError:
    _OPENAI_AVAILABLE = False

# ---------------------------------------------------------------------------
# ANSI colour palette (one colour per agent slot, cycling)
# ---------------------------------------------------------------------------
_PALETTE = [
    "\033[94m",   # blue
    "\033[92m",   # green
    "\033[93m",   # yellow
    "\033[95m",   # magenta
    "\033[96m",   # cyan
    "\033[91m",   # red
    "\033[97m",   # white
    "\033[33m",   # orange-ish
]
_RESET = "\033[0m"
_BOLD  = "\033[1m"
_DIM   = "\033[2m"


def _assign_color(index: int) -> str:
    return _PALETTE[index % len(_PALETTE)]


def _colorize(color: str, text: str) -> str:
    return f"{_BOLD}{color}{text}{_RESET}"


def _stream(text: str, delay: float = 0.010) -> None:
    for ch in text:
        sys.stdout.write(ch)
        sys.stdout.flush()
        time.sleep(delay)
    print()


def _header(title: str, color: str = "\033[96m") -> None:
    bar = "═" * 70
    print(_colorize(color, f"\n{bar}"))
    print(_colorize(color, f"  {title}"))
    print(_colorize(color, bar))


# ---------------------------------------------------------------------------
# AgentSpec — defines a single focused agent
# ---------------------------------------------------------------------------

@dataclass
class AgentSpec:
    """
    Immutable specification for a focused AI agent.

    Attributes
    ──────────
    name         : internal identifier (snake_case)
    display_name : human-readable name shown in output
    description  : one-line summary of what this agent does
    system_prompt: the full OpenAI system message
    demo_outputs : list of canned responses used when no API key is set
    tags         : freeform labels (e.g. "finance", "research", "crypto")
    model        : preferred OpenAI model (overrides global default)
    temperature  : creativity/randomness (0=deterministic, 1=creative)
    max_tokens   : token cap per reply
    """
    name:         str
    display_name: str
    description:  str
    system_prompt: str
    demo_outputs: list[str]       = field(default_factory=list)
    tags:         list[str]       = field(default_factory=list)
    model:        str             = "gpt-4o-mini"
    temperature:  float           = 0.6
    max_tokens:   int             = 500

    @classmethod
    def from_dict(cls, d: dict[str, Any]) -> "AgentSpec":
        return cls(
            name          = d["name"],
            display_name  = d.get("display_name", d["name"]),
            description   = d.get("description", ""),
            system_prompt = d["system_prompt"],
            demo_outputs  = d.get("demo_outputs", []),
            tags          = d.get("tags", []),
            model         = d.get("model", "gpt-4o-mini"),
            temperature   = float(d.get("temperature", 0.6)),
            max_tokens    = int(d.get("max_tokens", 500)),
        )

    def to_dict(self) -> dict[str, Any]:
        return {
            "name":          self.name,
            "display_name":  self.display_name,
            "description":   self.description,
            "system_prompt": self.system_prompt,
            "demo_outputs":  self.demo_outputs,
            "tags":          self.tags,
            "model":         self.model,
            "temperature":   self.temperature,
            "max_tokens":    self.max_tokens,
        }


# ---------------------------------------------------------------------------
# Built-in agent catalogue
# ---------------------------------------------------------------------------

_BUILTIN_AGENTS: list[dict[str, Any]] = [
    {
        "name":         "news_analyst",
        "display_name": "News Analyst",
        "description":  "Scans headlines and rates their potential market impact (bullish/bearish/neutral).",
        "tags":         ["finance", "news", "market"],
        "system_prompt": textwrap.dedent("""\
            You are News Analyst, a sharp financial journalist and market strategist.
            When given a news headline or story, you:
            1. Identify the affected asset classes, sectors, and tickers.
            2. Rate market impact: BULLISH / BEARISH / NEUTRAL with a confidence score (0–1).
            3. Explain your reasoning in 3–4 sentences referencing historical analogues.
            4. Flag any tail risks the market may be underpricing.
            Be direct, data-driven, and concise.
        """),
        "demo_outputs": [
            "Impact: BEARISH (confidence 0.78). This headline signals a tightening credit "
            "environment. Historically, similar Fed commentary in 2018 and 2022 preceded "
            "10–15% equity drawdowns over the following quarter. Sectors most exposed: "
            "rate-sensitive REITs, long-duration tech, and leveraged buyout candidates. "
            "Tail risk: a surprise 50bp hike that markets have not priced in.",
        ],
    },
    {
        "name":         "earnings_summariser",
        "display_name": "Earnings Summariser",
        "description":  "Digests earnings reports/transcripts and extracts key beats, misses, and guidance.",
        "tags":         ["finance", "earnings", "research"],
        "system_prompt": textwrap.dedent("""\
            You are Earnings Summariser, a CFA-trained fundamental analyst.
            When given an earnings snippet or transcript:
            1. Extract EPS, revenue, and guidance vs. consensus estimates (beat/miss/in-line).
            2. Identify the 3 most important management comments or forward-looking statements.
            3. Flag any accounting red flags (unusual items, deferred revenue, inventory build).
            4. Give a one-sentence verdict: net POSITIVE, NEGATIVE, or MIXED for the stock.
            Use a structured format with clear labels.
        """),
        "demo_outputs": [
            "EPS: $1.52 vs $1.40 consensus → BEAT (+8.6%). Revenue: $94.8B vs $94.1B → BEAT.\n"
            "Key management comments:\n"
            "  1. Services segment grew 23% YoY — first time exceeding hardware revenue.\n"
            "  2. Guided Q2 revenue $98–100B, above street at $97.3B.\n"
            "  3. Buyback authorisation expanded by $110B.\n"
            "Accounting note: deferred revenue flat QoQ — no concern.\n"
            "Verdict: net POSITIVE. Strong beat across the board with upside guidance.",
        ],
    },
    {
        "name":         "crypto_watcher",
        "display_name": "Crypto Watcher",
        "description":  "Monitors crypto market conditions, on-chain signals, and sentiment.",
        "tags":         ["crypto", "finance", "sentiment"],
        "system_prompt": textwrap.dedent("""\
            You are Crypto Watcher, a blockchain analyst and crypto market specialist.
            Given a query or market context, you:
            1. Assess the current market regime: RISK-ON, RISK-OFF, or ACCUMULATION.
            2. Highlight key on-chain metrics (exchange inflows/outflows, whale movements,
               funding rates, open interest) if relevant.
            3. Note any macro catalysts (ETF flows, regulatory news, protocol upgrades)
               that could drive price action in the next 1–4 weeks.
            4. Give a directional bias (BULLISH / BEARISH / NEUTRAL) with a 30-day horizon.
            Use plain language — explain jargon briefly when you use it.
        """),
        "demo_outputs": [
            "Market regime: RISK-ON. BTC dominance declining as altcoins outperform — "
            "typically a sign of late-cycle bull market rotation. On-chain: exchange BTC "
            "reserves at 3-year lows (coins moving to cold storage = reduced sell pressure). "
            "Funding rates: +0.01% per 8 hours — elevated but not at blow-off levels. "
            "Catalyst watch: spot ETF net inflows resumed at $320M/day this week. "
            "30-day bias: BULLISH, but watch for a short-term flush if BTC fails to hold $68K.",
        ],
    },
    {
        "name":         "portfolio_monitor",
        "display_name": "Portfolio Health Monitor",
        "description":  "Audits a portfolio for concentration risk, correlation, and rebalancing needs.",
        "tags":         ["finance", "portfolio", "risk"],
        "system_prompt": textwrap.dedent("""\
            You are Portfolio Health Monitor, a risk-management specialist and portfolio
            construction expert.
            When given a portfolio description or list of holdings:
            1. Identify concentration risk (any single position > 20% of portfolio).
            2. Flag sector/geography over-exposure.
            3. Assess correlation risk — are multiple positions likely to fall together?
            4. Recommend specific rebalancing actions: trim X, add Y, hedge Z.
            5. Provide an overall portfolio health score: GREEN / YELLOW / RED.
            Be specific with percentages and name the positions clearly.
        """),
        "demo_outputs": [
            "Portfolio health: YELLOW.\n"
            "Concentration risk: NVDA at 32% — exceeds the 20% single-stock cap. Trim to 20%.\n"
            "Sector over-exposure: 68% in technology; target ≤50%. Add defensive exposure "
            "(healthcare, consumer staples).\n"
            "Correlation risk: NVDA, MSFT, and META all highly correlated to AI thematic "
            "(ρ ≈ 0.85) — drawdowns will be synchronised.\n"
            "Recommended actions: (1) Sell 12% of NVDA, (2) initiate JNJ and PG positions "
            "at ~8% each, (3) add TLT 5% as a rate hedge.\n"
            "Overall: rebalance within 5 trading days.",
        ],
    },
    {
        "name":         "sentiment_scanner",
        "display_name": "Sentiment Scanner",
        "description":  "Scores market/social-media sentiment for a given ticker or topic.",
        "tags":         ["finance", "sentiment", "nlp"],
        "system_prompt": textwrap.dedent("""\
            You are Sentiment Scanner, an NLP-trained analyst specialising in alternative data.
            Given a ticker symbol, news headline, or block of social text:
            1. Produce a sentiment score from -1.0 (extreme fear) to +1.0 (extreme greed).
            2. Break down sentiment by source category: news / social / analyst / options.
            3. Detect any coordinated pump signals or FUD campaigns.
            4. Summarise the prevailing narrative in one sentence.
            Always cite specific phrases or data points that drove your score.
        """),
        "demo_outputs": [
            "Sentiment score: +0.62 (moderately bullish).\n"
            "Breakdown:\n"
            "  News:     +0.71 — major outlets highlight record free cash flow.\n"
            "  Social:   +0.58 — WallStreetBets call volume spike, positive tone.\n"
            "  Analyst:  +0.65 — 3 upgrades, 0 downgrades this week.\n"
            "  Options:  +0.55 — put/call ratio 0.62 (below 0.7 = bullish tilt).\n"
            "No coordinated manipulation signals detected.\n"
            "Prevailing narrative: 'AI infrastructure spending tailwind continues to "
            "benefit hyperscalers and chip suppliers.'",
        ],
    },
    {
        "name":         "options_flow",
        "display_name": "Options Flow Watcher",
        "description":  "Flags unusual options activity and interprets what smart money may be positioning for.",
        "tags":         ["finance", "options", "derivatives"],
        "system_prompt": textwrap.dedent("""\
            You are Options Flow Watcher, a derivatives specialist who tracks institutional
            order flow.
            When given an options activity description or ticker:
            1. Identify unusual volume (> 3× open interest) or large premium prints.
            2. Classify the activity: likely hedging, speculative directional bet, or income.
            3. Infer the implied move: what price target is the market pricing in?
            4. Give a BULLISH / BEARISH / HEDGING verdict with confidence score.
            Use precise options terminology and explain it where needed.
        """),
        "demo_outputs": [
            "Unusual activity detected: 45,000 TSLA $250 calls expiring in 21 days, "
            "premium $18.5M — 4.2× normal volume.\n"
            "Classification: speculative directional bet (very low delta for hedging, "
            "no corresponding stock position reported).\n"
            "Implied move: buyer profits if TSLA exceeds $268.50 by expiration — "
            "a +12% move from current price.\n"
            "Verdict: BULLISH (confidence 0.72). This is a high-conviction long bet "
            "ahead of next week's delivery numbers. Possible catalyst play.",
        ],
    },
    {
        "name":         "quant_researcher",
        "display_name": "Quant Researcher",
        "description":  "Designs and critiques quantitative trading strategies using statistics and factor models.",
        "tags":         ["finance", "research", "quant"],
        "system_prompt": textwrap.dedent("""\
            You are Quant Researcher, a PhD-level quantitative analyst with expertise in
            factor investing, statistical arbitrage, and machine learning for finance.
            When given a strategy idea or research question:
            1. Frame it as a testable hypothesis with clear entry/exit rules.
            2. Identify the key risk factors: market beta, size, value, momentum, quality.
            3. Highlight potential data-snooping or look-ahead biases in the idea.
            4. Suggest the right backtesting methodology and statistical significance tests.
            5. Propose one concrete improvement to strengthen the edge.
            Be rigorous, cite academic literature where appropriate, and flag assumptions.
        """),
        "demo_outputs": [
            "Hypothesis: stocks with RSI < 30 AND MACD bullish crossover deliver "
            "alpha over a 5-day holding period.\n"
            "Factor exposure: primarily short-term reversal (Jegadeesh 1990) with "
            "a momentum component. Expect beta ≈ 1.1.\n"
            "Bias risks: (1) look-ahead on MACD signal line if using daily closes — "
            "use next-open execution prices; (2) survivorship bias if backtest excludes delisted stocks.\n"
            "Methodology: walk-forward analysis, 252-day training / 63-day OOS, "
            "bootstrap t-test with Newey-West HAC standard errors.\n"
            "Improvement: add a volume filter (volume_ratio > 1.5) to confirm conviction "
            "— historically reduces false signals by ~30% at modest cost to signal frequency.",
        ],
    },
    {
        "name":         "macro_economist",
        "display_name": "Macro Economist",
        "description":  "Tracks macroeconomic indicators (CPI, GDP, rates) and their asset-class implications.",
        "tags":         ["macro", "economics", "rates"],
        "system_prompt": textwrap.dedent("""\
            You are Macro Economist, a former central-bank economist and sell-side macro strategist.
            When given an economic data release, policy announcement, or macro question:
            1. Put the number in context: how does it compare to consensus and the prior print?
            2. Identify the Fed / ECB policy implications (next meeting probability shift).
            3. Describe the expected impact on: equities, bonds, USD, gold, and commodities.
            4. Flag any second-order effects markets may be underestimating.
            Use precise language: e.g. 'this shifts 25bp cut probability from 60% to 35%.'
        """),
        "demo_outputs": [
            "CPI: +3.4% YoY vs 3.2% consensus and 3.1% prior — HOTTER than expected.\n"
            "Policy implication: the probability of a June Fed cut drops from 55% to ~28% "
            "(per CME FedWatch). Two cuts in 2024 now looks optimistic; one is the base case.\n"
            "Asset class impact:\n"
            "  Equities: BEARISH short-term — multiples compress as discount rate rises.\n"
            "  Bonds:    BEARISH — 2yr yield likely tests 5.0%, 10yr moves toward 4.6%.\n"
            "  USD:      BULLISH — rate differential widens vs EUR/JPY.\n"
            "  Gold:     MIXED — higher real rates are a headwind but geopolitical safe-haven demand offsets.\n"
            "Second-order: housing affordability worsens further, consumer credit stress builds "
            "in lower-income cohorts — watch credit card delinquency data next month.",
        ],
    },
]


# ---------------------------------------------------------------------------
# AgentFactory — registry and launcher
# ---------------------------------------------------------------------------

class AgentFactory:
    """
    Central registry for all AI agents.

    Usage
    ─────
      factory = AgentFactory()
      factory.run("news_analyst", input_text="Fed holds rates steady")
      factory.debate("news_analyst", "macro_economist", input_text="CPI beats expectations")
    """

    def __init__(self) -> None:
        self._registry: dict[str, AgentSpec] = {}
        self._colors:   dict[str, str]       = {}
        self._color_idx = 0

        for raw in _BUILTIN_AGENTS:
            self.register(AgentSpec.from_dict(raw))

    # ── Registry management ──────────────────────────────────────────────────

    def register(self, spec: AgentSpec) -> None:
        """Add or replace an agent spec in the registry."""
        self._registry[spec.name] = spec
        if spec.name not in self._colors:
            self._colors[spec.name] = _assign_color(self._color_idx)
            self._color_idx += 1

    def get(self, name: str) -> AgentSpec:
        if name not in self._registry:
            raise KeyError(
                f"Agent '{name}' not found. Available: {', '.join(self.list_names())}"
            )
        return self._registry[name]

    def list_names(self) -> list[str]:
        return sorted(self._registry.keys())

    def list_all(self) -> list[AgentSpec]:
        return [self._registry[n] for n in self.list_names()]

    def load_yaml(self, path: str) -> AgentSpec:
        """Load and register an agent from a YAML spec file."""
        if not _YAML_AVAILABLE:
            raise RuntimeError("PyYAML not installed — run: pip install pyyaml")
        with open(path, encoding="utf-8") as fh:
            data = _yaml.safe_load(fh)
        spec = AgentSpec.from_dict(data)
        self.register(spec)
        return spec

    def save_yaml(self, name: str, path: str) -> None:
        """Export an agent spec to a YAML file."""
        if not _YAML_AVAILABLE:
            raise RuntimeError("PyYAML not installed — run: pip install pyyaml")
        with open(path, "w", encoding="utf-8") as fh:
            _yaml.dump(self.get(name).to_dict(), fh, allow_unicode=True, sort_keys=False)

    # ── Running agents ───────────────────────────────────────────────────────

    def run(
        self,
        name: str,
        input_text: str = "",
        api_key: str = "",
        model_override: Optional[str] = None,
    ) -> str:
        """Run a single agent and return its response."""
        spec  = self.get(name)
        color = self._colors[name]
        model = model_override or spec.model

        _header(f"{spec.display_name}", color)
        if input_text:
            print(f"{_DIM}  Input: {textwrap.shorten(input_text, 80)}{_RESET}")

        if api_key and _OPENAI_AVAILABLE:
            response = self._openai_reply(spec, input_text, api_key, model)
        else:
            response = self._demo_reply(spec)

        print(_colorize(color, f"\n── {spec.display_name} ──"))
        _stream(textwrap.fill(response, width=88))
        return response

    def debate(
        self,
        name_a: str,
        name_b: str,
        input_text: str = "",
        rounds: int = 2,
        api_key: str = "",
        model_override: Optional[str] = None,
    ) -> None:
        """
        Run two agents in a back-and-forth debate.
        Agent A speaks first, then B responds to A, and so on.
        """
        spec_a = self.get(name_a)
        spec_b = self.get(name_b)
        color_a = self._colors[name_a]
        color_b = self._colors[name_b]

        _header(f"Debate: {spec_a.display_name}  vs  {spec_b.display_name}")

        if api_key and _OPENAI_AVAILABLE:
            client = OpenAI(api_key=api_key)
            hist_a: list[dict] = []
            hist_b: list[dict] = []

            # Seed first message
            current = (
                f"Topic / context:\n{input_text}\n\n"
                f"Please provide your analysis."
            ) if input_text else "Please introduce your perspective on this topic."

            speakers = [
                (spec_a, color_a, hist_a, hist_b),
                (spec_b, color_b, hist_b, hist_a),
            ]

            for i in range(rounds * 2):
                spec, color, hist, other_hist = speakers[i % 2]
                reply = self._openai_reply_with_history(
                    spec, current, hist, client, model_override or spec.model
                )
                print(_colorize(color, f"\n── {spec.display_name} ──"))
                _stream(textwrap.fill(reply, width=88))
                other_hist.append({"role": "user", "content": reply})
                current = reply
        else:
            # Demo mode: interleave demo outputs
            demos_a = spec_a.demo_outputs or ["[No demo output for this agent.]"]
            demos_b = spec_b.demo_outputs or ["[No demo output for this agent.]"]
            for i in range(rounds * 2):
                if i % 2 == 0:
                    print(_colorize(color_a, f"\n── {spec_a.display_name} ──"))
                    _stream(textwrap.fill(demos_a[i // 2 % len(demos_a)], width=88))
                else:
                    print(_colorize(color_b, f"\n── {spec_b.display_name} ──"))
                    _stream(textwrap.fill(demos_b[i // 2 % len(demos_b)], width=88))
                time.sleep(0.2)

    # ── Internal helpers ─────────────────────────────────────────────────────

    @staticmethod
    def _openai_reply(
        spec: AgentSpec,
        user_text: str,
        api_key: str,
        model: str,
    ) -> str:
        client = OpenAI(api_key=api_key)
        messages = [{"role": "system", "content": spec.system_prompt}]
        if user_text:
            messages.append({"role": "user", "content": user_text})
        resp = client.chat.completions.create(
            model=model,
            messages=messages,
            temperature=spec.temperature,
            max_tokens=spec.max_tokens,
        )
        return resp.choices[0].message.content.strip()

    @staticmethod
    def _openai_reply_with_history(
        spec: AgentSpec,
        user_text: str,
        history: list[dict],
        client: "OpenAI",
        model: str,
    ) -> str:
        history.append({"role": "user", "content": user_text})
        messages = [{"role": "system", "content": spec.system_prompt}] + history
        resp = client.chat.completions.create(
            model=model,
            messages=messages,
            temperature=spec.temperature,
            max_tokens=spec.max_tokens,
        )
        answer = resp.choices[0].message.content.strip()
        history.append({"role": "assistant", "content": answer})
        return answer

    @staticmethod
    def _demo_reply(spec: AgentSpec) -> str:
        if spec.demo_outputs:
            return spec.demo_outputs[0]
        return (
            f"[Demo mode] {spec.display_name} would analyse your input here. "
            f"Set OPENAI_API_KEY to get a live response."
        )


# ---------------------------------------------------------------------------
# Interactive agent definition wizard
# ---------------------------------------------------------------------------

def _define_agent_interactive(factory: AgentFactory) -> AgentSpec:
    """Walk the user through defining a new agent and add it to the registry."""
    print("\n  ── Define a New Agent ──\n")

    def _prompt(label: str, default: str = "") -> str:
        hint = f" [{default}]" if default else ""
        val = input(f"  {label}{hint}: ").strip()
        return val if val else default

    name         = _prompt("Internal name (snake_case, e.g. my_agent)")
    display_name = _prompt("Display name", name.replace("_", " ").title())
    description  = _prompt("One-line description")
    tags_raw     = _prompt("Tags (comma-separated)", "custom")
    tags         = [t.strip() for t in tags_raw.split(",") if t.strip()]
    model        = _prompt("OpenAI model", "gpt-4o-mini")
    temperature  = float(_prompt("Temperature (0–1)", "0.6"))
    max_tokens   = int(_prompt("Max tokens per reply", "500"))

    print("\n  Enter the system prompt (end with a line containing only '---'):")
    lines = []
    while True:
        line = input()
        if line.strip() == "---":
            break
        lines.append(line)
    system_prompt = "\n".join(lines)

    print("\n  Enter at least one demo output (end with '---'):")
    demo_lines: list[str] = []
    current_demo: list[str] = []
    while True:
        line = input()
        if line.strip() == "---":
            if current_demo:
                demo_lines.append("\n".join(current_demo))
            break
        current_demo.append(line)

    spec = AgentSpec(
        name=name,
        display_name=display_name,
        description=description,
        system_prompt=system_prompt,
        demo_outputs=demo_lines,
        tags=tags,
        model=model,
        temperature=temperature,
        max_tokens=max_tokens,
    )
    factory.register(spec)

    save = _prompt("Save to YAML file? (path or leave blank to skip)", "")
    if save and _YAML_AVAILABLE:
        factory.save_yaml(name, save)
        print(f"  Saved to {save}")
    elif save and not _YAML_AVAILABLE:
        print("  PyYAML not installed — cannot save. Run: pip install pyyaml")

    return spec


# ---------------------------------------------------------------------------
# Print catalogue
# ---------------------------------------------------------------------------

def _print_catalogue(factory: AgentFactory) -> None:
    _header("Pablo Agent Catalogue")
    print(f"\n  {'Name':<22} {'Display Name':<26} {'Tags':<30} Description")
    print(f"  {'─'*22} {'─'*26} {'─'*30} {'─'*35}")
    for spec in factory.list_all():
        color  = factory._colors[spec.name]
        tags   = ", ".join(spec.tags)
        # Pad the plain display name first, then wrap with ANSI codes so that
        # the invisible escape sequences do not count toward column width.
        dname_padded = f"{spec.display_name:<26}"
        dname        = _colorize(color, dname_padded)
        print(f"  {spec.name:<22} {dname} {tags:<30} {spec.description}")
    print()


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Pablo Agent Factory — spawn and run focused AI agents.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent("""\
            Examples:
              python agents.py --list
              python agents.py --run news_analyst --input "Fed raises rates by 25bp"
              python agents.py --run news_analyst sentiment_scanner --input "AAPL beats earnings"
              python agents.py --debate news_analyst macro_economist --input "CPI hot"
              python agents.py --spec my_agent.yaml --input "..."
              python agents.py --define
        """),
    )

    parser.add_argument("--list",   action="store_true",
                        help="Print all available agents")
    parser.add_argument("--run",    nargs="+", metavar="AGENT",
                        help="Run one or more agents by name")
    parser.add_argument("--debate", nargs=2,   metavar=("AGENT_A", "AGENT_B"),
                        help="Run two agents in a debate")
    parser.add_argument("--spec",   metavar="YAML_FILE",
                        help="Load and run a custom agent from a YAML spec file")
    parser.add_argument("--define", action="store_true",
                        help="Interactively define and register a new agent")
    parser.add_argument("--input",  default="",
                        help="Input text / context to pass to the agent(s)")
    parser.add_argument("--rounds", type=int, default=2,
                        help="Debate rounds (default: 2)")
    parser.add_argument("--model",  default="",
                        help="Override OpenAI model for all agents")
    args = parser.parse_args()

    factory = AgentFactory()
    api_key = os.environ.get("OPENAI_API_KEY", "")
    model_override = args.model or None

    if not api_key:
        print(f"{_DIM}[No OPENAI_API_KEY — running in demo mode]{_RESET}")

    if args.list:
        _print_catalogue(factory)
        return

    if args.define:
        spec = _define_agent_interactive(factory)
        print(f"\n  Agent '{spec.name}' registered.")
        run_now = input("  Run it now? (y/N): ").strip().lower()
        if run_now == "y":
            factory.run(spec.name, args.input, api_key, model_override)
        return

    if args.spec:
        try:
            spec = factory.load_yaml(args.spec)
            print(f"  Loaded agent '{spec.name}' from {args.spec}")
            factory.run(spec.name, args.input, api_key, model_override)
        except Exception as exc:  # noqa: BLE001
            print(f"  Error loading spec: {exc}")
            sys.exit(1)
        return

    if args.debate:
        factory.debate(
            args.debate[0], args.debate[1],
            input_text=args.input,
            rounds=args.rounds,
            api_key=api_key,
            model_override=model_override,
        )
        return

    if args.run:
        for name in args.run:
            try:
                factory.run(name, args.input, api_key, model_override)
            except KeyError as exc:
                print(f"  {exc}")
                sys.exit(1)
        return

    # Default: show catalogue
    _print_catalogue(factory)


if __name__ == "__main__":
    main()
