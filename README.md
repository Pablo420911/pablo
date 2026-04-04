# Pablo

**Pablo** — a dual-AI quantum mechanics teaching system.

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
