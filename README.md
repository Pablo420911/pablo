# Pablo

**Pablo** — a dual-AI quantum mechanics teaching system, automated trading platform, and extensible AI agent factory.

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

---

## AI Agent Factory — `agents.py`

A dynamic registry that lets you **define, spawn, and run any number of focused AI agents** — each with its own persona, tools, and task loop.  Works in demo mode with no API key; pass `OPENAI_API_KEY` for live AI responses.

### Built-in agents

| Name | Role |
|---|---|
| `news_analyst` | Scans headlines, rates market impact (BULLISH/BEARISH/NEUTRAL) |
| `earnings_summariser` | Extracts EPS beats/misses, guidance, and red flags from earnings reports |
| `crypto_watcher` | Monitors crypto market regime, on-chain signals, and 30-day bias |
| `portfolio_monitor` | Audits holdings for concentration risk and recommends rebalancing |
| `sentiment_scanner` | Scores news/social/analyst/options sentiment -1.0 → +1.0 |
| `options_flow` | Flags unusual options activity and infers smart-money positioning |
| `quant_researcher` | Designs and critiques quantitative strategies with academic rigor |
| `macro_economist` | Puts CPI/GDP/rate data in context and maps asset-class implications |

### Quick commands

```bash
# List all available agents:
python agents.py --list

# Run a single agent (demo mode, no key needed):
python agents.py --run news_analyst --input "Fed raises rates 25bp"

# Run multiple agents in sequence:
python agents.py --run news_analyst sentiment_scanner --input "Tesla misses deliveries"

# Debate between two agents:
python agents.py --debate news_analyst macro_economist --input "CPI hot at 3.4%"

# Use live AI:
export OPENAI_API_KEY="sk-..."
python agents.py --run earnings_summariser --input "AAPL Q1 EPS $1.52 vs $1.40 est."
```

### Add your own agent

**Option 1 — interactive wizard:**
```bash
python agents.py --define
```

**Option 2 — YAML spec file:**
```yaml
# my_agent.yaml
name: my_agent
display_name: My Custom Agent
description: Does something very specific.
tags: [custom, research]
model: gpt-4o-mini
temperature: 0.6
max_tokens: 500
system_prompt: |
  You are My Custom Agent. When given input you will...
demo_outputs:
  - "This is what I would say in demo mode."
```
```bash
python agents.py --spec my_agent.yaml --input "my question"
```

**Option 3 — Python API:**
```python
from agents import AgentFactory, AgentSpec

factory = AgentFactory()
factory.register(AgentSpec(
    name="my_agent",
    display_name="My Agent",
    description="Does something focused.",
    system_prompt="You are a specialist in...",
    demo_outputs=["Demo response here."],
    tags=["custom"],
))
factory.run("my_agent", input_text="...", api_key="sk-...")
```

### Agent options

| Flag | Description |
|---|---|
| `--list` | Print all registered agents |
| `--run AGENT [AGENT ...]` | Run one or more agents by name |
| `--debate AGENT_A AGENT_B` | Two agents debate the input |
| `--spec YAML_FILE` | Load and run a custom agent from YAML |
| `--define` | Interactive wizard to create a new agent |
| `--input TEXT` | Context/question to pass to the agent(s) |
| `--rounds N` | Debate rounds (default: 2) |
| `--model MODEL` | Override the OpenAI model for all agents |
