# Pablo

**Pablo** — a dual-AI quantum mechanics teaching system and automated trading platform.

Two AI agents, **Professor Quantum** and **Curious Quantum**, hold a live
back-and-forth conversation that teaches quantum mechanics to anyone reading along.

---

## Quick start

### With an OpenAI API key (live AI)

```bash
pip install -r requirements.txt
export OPENAI_API_KEY="sk-..."
python quantum_chat.py
```

### Without an API key (demo mode)

```bash
python quantum_chat.py          # no install needed — uses a built-in transcript
```

---

## Options

| Flag | Default | Description |
|---|---|---|
| `--topic` | wave-particle duality intro | Quantum topic for the agents to discuss |
| `--rounds` | `6` | Number of conversational turns |
| `--model` | `gpt-4o-mini` | OpenAI model to use |

**Example — deep-dive into entanglement, 10 turns:**

```bash
python quantum_chat.py --topic "quantum entanglement and Bell's theorem" --rounds 10
```

---

## How it works

| Agent | Role |
|---|---|
| **Professor Quantum** | Explains concepts with vivid analogies, ends each turn with a question |
| **Curious Quantum** | Asks insightful follow-ups, connects ideas, keeps the dialogue going |

Each agent maintains its own conversation history so the discussion stays coherent across many turns. When no API key is set the script plays back a rich built-in transcript covering superposition, decoherence, quantum computing, entanglement, Bell's theorem, and interpretations of quantum mechanics.

---

## Automated Trading — `trader.py`

Two AI agents debate every potential trade before a single order is placed:

| Agent | Role |
|---|---|
| **Analyst AI** | Reads technical indicators, builds a BUY/SELL/HOLD thesis with position size |
| **Risk Manager AI** | Stress-tests the thesis, checks reward-to-risk ratio, approves/rejects/modifies |

The system then executes via **paper simulation** (default, no money) or a **live Alpaca brokerage account**.

### Install dependencies

```bash
pip install -r requirements.txt
```

### Paper simulation (no credentials needed)

```bash
python trader.py --symbols AAPL MSFT TSLA --cash 10000
```

### With AI debate (requires OpenAI key)

```bash
export OPENAI_API_KEY="sk-..."
python trader.py --symbols NVDA TSLA --cash 50000 --rounds 3
```

### Alpaca paper brokerage (free account at [alpaca.markets](https://alpaca.markets))

```bash
export ALPACA_API_KEY="..."
export ALPACA_SECRET_KEY="..."
export ALPACA_BASE_URL="https://paper-api.alpaca.markets"
python trader.py --symbols AAPL --broker alpaca
```

### Live trading (real money — use with caution)

```bash
export ALPACA_BASE_URL="https://api.alpaca.markets"
python trader.py --symbols AAPL --broker alpaca --live
```

### Trading options

| Flag | Default | Description |
|---|---|---|
| `--symbols` | `AAPL MSFT TSLA` | Ticker symbols to analyse and trade |
| `--cash` | `10000` | Starting cash for paper simulation |
| `--model` | `gpt-4o-mini` | OpenAI model for the AI debate |
| `--rounds` | `2` | Debate rounds per symbol |
| `--broker` | `paper` | `paper` (local simulation) or `alpaca` |
| `--live` | off | Enable real-money execution via Alpaca |

### How the trading pipeline works

1. **Market data** — downloads 3 months of OHLCV data via `yfinance` and computes RSI, MACD, Bollinger Bands, SMA-20/50, ATR, and volume ratio.
2. **AI debate** — Analyst AI builds a trade thesis; Risk Manager AI stress-tests it over multiple rounds until consensus is reached.
3. **Rule-based fallback** — when no OpenAI key is set, a classical multi-indicator scoring model generates the signal automatically.
4. **Investment plan** — prints a structured plan showing dollar allocation, entry timing advice, stop-loss, and take-profit levels for every symbol.
5. **Execution** — orders are placed via the chosen backend (paper simulation or Alpaca broker API) with bracket orders (stop-loss + take-profit) attached automatically.

> ⚠️ **Disclaimer:** This software is for educational purposes only. Past performance does not guarantee future results. Never invest money you cannot afford to lose.
