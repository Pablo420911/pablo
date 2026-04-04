"""
quantum_chat.py

Two AI agents — Professor Quantum and Curious Quantum — converse with each other
and teach quantum mechanics concepts. The conversation is printed live so the
user can follow along and learn.

Usage:
    export OPENAI_API_KEY="sk-..."
    python quantum_chat.py [--topic "wave-particle duality"] [--rounds 6]

If no API key is found the script falls back to a built-in demo transcript so
you can still see what the output looks like without any credentials.
"""

import argparse
import os
import sys
import textwrap
import time

# ---------------------------------------------------------------------------
# Optional OpenAI import
# ---------------------------------------------------------------------------
try:
    from openai import OpenAI
    _OPENAI_AVAILABLE = True
except ImportError:
    _OPENAI_AVAILABLE = False


# ---------------------------------------------------------------------------
# Agent definitions
# ---------------------------------------------------------------------------

PROFESSOR_SYSTEM = textwrap.dedent("""\
    You are Professor Quantum, an enthusiastic and patient quantum physics expert.
    Your job is to explain quantum mechanics concepts clearly, using vivid analogies
    and concrete examples. Keep each response to 3–5 sentences. End every response
    with a thought-provoking follow-up question aimed at your conversation partner,
    Curious Quantum, to deepen the discussion.
""")

CURIOUS_SYSTEM = textwrap.dedent("""\
    You are Curious Quantum, an eager AI student who loves quantum mechanics.
    When Professor Quantum explains something, you ask insightful follow-up
    questions, make connections to related concepts, and occasionally share a
    short interesting fact you "know". Keep each response to 3–5 sentences and
    always end with a question back to Professor Quantum so the conversation
    continues naturally.
""")

AGENT_NAMES = {
    "professor": "Professor Quantum",
    "curious":   "Curious Quantum",
}

COLORS = {
    "professor": "\033[94m",   # blue
    "curious":   "\033[92m",   # green
    "reset":     "\033[0m",
    "bold":      "\033[1m",
    "dim":       "\033[2m",
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _colorize(role: str, text: str) -> str:
    color = COLORS.get(role, "")
    reset = COLORS["reset"]
    bold  = COLORS["bold"]
    return f"{bold}{color}{text}{reset}"


def _print_message(role: str, text: str) -> None:
    name = AGENT_NAMES[role]
    header = _colorize(role, f"\n── {name} ──")
    print(header)
    # Wrap long lines for readability
    for line in text.splitlines():
        print(textwrap.fill(line, width=88) if line.strip() else "")


def _stream_char(text: str, delay: float = 0.012) -> None:
    """Print text character-by-character to simulate a live conversation."""
    for ch in text:
        sys.stdout.write(ch)
        sys.stdout.flush()
        time.sleep(delay)
    print()


# ---------------------------------------------------------------------------
# OpenAI-backed agents
# ---------------------------------------------------------------------------

class QuantumAgent:
    """A single AI agent backed by the OpenAI Chat Completions API."""

    def __init__(self, role: str, system_prompt: str, client: "OpenAI", model: str = "gpt-4o-mini"):
        self.role          = role
        self.system_prompt = system_prompt
        self.client        = client
        self.model         = model
        self.history: list[dict] = []

    def _build_messages(self) -> list[dict]:
        return [{"role": "system", "content": self.system_prompt}] + self.history

    def reply(self, incoming: str) -> str:
        """Feed `incoming` text and return this agent's response."""
        self.history.append({"role": "user", "content": incoming})
        response = self.client.chat.completions.create(
            model=self.model,
            messages=self._build_messages(),
            temperature=0.8,
            max_tokens=300,
        )
        answer = response.choices[0].message.content.strip()
        self.history.append({"role": "assistant", "content": answer})
        return answer


# ---------------------------------------------------------------------------
# Demo transcript (no API key required)
# ---------------------------------------------------------------------------

DEMO_TRANSCRIPT = [
    (
        "professor",
        "Quantum mechanics is the branch of physics that governs the behavior of "
        "matter and energy at the smallest scales — atoms, electrons, and photons. "
        "One of its strangest ideas is *superposition*: a particle can exist in "
        "multiple states simultaneously until we observe it, at which point the "
        "wave function 'collapses' to a single outcome. Think of it like a coin "
        "spinning in the air — it's neither heads nor tails until it lands. "
        "What aspect of superposition would you like to explore first, Curious Quantum?",
    ),
    (
        "curious",
        "That spinning-coin analogy is brilliant! I've always wondered — does the "
        "act of 'observation' literally require a conscious observer, or is any "
        "physical interaction enough to collapse the wave function? "
        "I recall that even a single photon bouncing off a particle counts as a "
        "measurement. Does that mean the universe is constantly measuring itself? "
        "Professor, what does the Copenhagen interpretation say about this?",
    ),
    (
        "professor",
        "Great question! In the Copenhagen interpretation, *any* interaction that "
        "entangles the particle with the environment counts as a measurement — "
        "consciousness is not required. This process is called *decoherence*: the "
        "quantum system leaks information into its surroundings, destroying the "
        "superposition. So yes, in a very real sense, particles are being "
        "'measured' by the environment all the time, which is why we don't see "
        "everyday objects in superposition. How do you think decoherence relates "
        "to the difficulty of building quantum computers, Curious Quantum?",
    ),
    (
        "curious",
        "Oh, that connection is crystal clear now! Quantum computers need qubits "
        "to stay in superposition long enough to perform calculations, but "
        "decoherence destroys that superposition the moment the qubit interacts "
        "with anything in its environment. Engineers go to incredible lengths — "
        "cooling chips to near absolute zero — just to slow decoherence down. "
        "I know that error-correction codes also help compensate for decoherence "
        "errors. Does quantum entanglement play a role in those error-correction "
        "strategies, Professor?",
    ),
    (
        "professor",
        "Absolutely — entanglement is at the heart of quantum error correction! "
        "By entangling several physical qubits together to represent one logical "
        "qubit, we can detect and fix errors without directly measuring (and thus "
        "collapsing) the logical qubit's state. It's a clever workaround: we "
        "measure *correlations* between qubits rather than the qubits themselves. "
        "This is analogous to using redundancy in classical error correction, but "
        "with the added twist of non-local quantum correlations. Speaking of "
        "non-locality — what's your intuition about Bell's theorem and what it "
        "tells us about the nature of reality?",
    ),
    (
        "curious",
        "Bell's theorem is mind-bending! It proves that no 'hidden variable' theory "
        "can reproduce all the predictions of quantum mechanics while also being "
        "local — meaning the results of measurements on entangled particles are "
        "correlated in ways that can't be explained by any pre-existing local "
        "information. Experiments like the Aspect experiment in 1982 confirmed "
        "these predictions with stunning precision. So either the world is "
        "non-local, or we have to abandon the idea that particles have definite "
        "properties before we measure them. Professor, how do different "
        "interpretations of quantum mechanics — like Many-Worlds — handle this "
        "non-locality puzzle?",
    ),
]


# ---------------------------------------------------------------------------
# Main conversation loop
# ---------------------------------------------------------------------------

def run_live_conversation(topic: str, rounds: int, api_key: str, model: str) -> None:
    """Drive a live AI-to-AI conversation via the OpenAI API."""
    client = OpenAI(api_key=api_key)
    professor = QuantumAgent("professor", PROFESSOR_SYSTEM, client, model)
    curious   = QuantumAgent("curious",   CURIOUS_SYSTEM,   client, model)

    print(_colorize("professor", f"\n{'═'*70}"))
    print(_colorize("professor", f"  Quantum Mechanics — AI-to-AI Teaching Session"))
    print(_colorize("professor", f"  Topic: {topic}"))
    print(_colorize("professor", f"{'═'*70}"))

    # Seed the conversation with the chosen topic
    current_message = (
        f"Let's have an educational conversation about quantum mechanics. "
        f"Please start by introducing the topic: {topic}."
    )

    # Alternate between professor and curious
    roles = ["professor", "curious"]
    agents = {"professor": professor, "curious": curious}

    for i in range(rounds):
        speaker_role   = roles[i % 2]
        listener_role  = roles[(i + 1) % 2]
        agent          = agents[speaker_role]

        response = agent.reply(current_message)

        # Print with streaming effect
        header = _colorize(speaker_role, f"\n── {AGENT_NAMES[speaker_role]} ──")
        print(header)
        _stream_char(textwrap.fill(response, width=88))

        # The other agent receives this as its next prompt
        agents[listener_role].history.append({"role": "user", "content": response})
        current_message = response

    print(_colorize("professor", f"\n{'═'*70}\n"))


def run_demo_conversation() -> None:
    """Play back the built-in demo transcript when no API key is available."""
    print(_colorize("professor", f"\n{'═'*70}"))
    print(_colorize("professor", "  Quantum Mechanics — AI-to-AI Teaching Session  [DEMO MODE]"))
    print(_colorize("professor", "  (Set OPENAI_API_KEY to enable live AI responses)"))
    print(_colorize("professor", f"{'═'*70}"))

    for role, text in DEMO_TRANSCRIPT:
        _print_message(role, "")
        _stream_char(textwrap.fill(text, width=88))
        time.sleep(0.3)

    print(_colorize("professor", f"\n{'═'*70}\n"))


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Two AI agents teach quantum mechanics to each other (and you).",
    )
    parser.add_argument(
        "--topic",
        default="the fundamentals of quantum mechanics, starting with wave-particle duality",
        help="Quantum mechanics topic for the agents to discuss",
    )
    parser.add_argument(
        "--rounds",
        type=int,
        default=6,
        help="Number of conversational turns (default: 6)",
    )
    parser.add_argument(
        "--model",
        default="gpt-4o-mini",
        help="OpenAI model to use (default: gpt-4o-mini)",
    )
    args = parser.parse_args()

    api_key = os.environ.get("OPENAI_API_KEY", "")

    if api_key and _OPENAI_AVAILABLE:
        run_live_conversation(args.topic, args.rounds, api_key, args.model)
    else:
        if not _OPENAI_AVAILABLE:
            print(
                f"{COLORS['dim']}[openai package not installed — "
                f"running demo mode. Install with: pip install openai]{COLORS['reset']}"
            )
        elif not api_key:
            print(
                f"{COLORS['dim']}[No OPENAI_API_KEY found — "
                f"running demo mode. Set the env var to enable live AI.]{COLORS['reset']}"
            )
        run_demo_conversation()


if __name__ == "__main__":
    main()
