# Pablo – Home AI Assistant

Pablo is a **C++ AI assistant** designed to help manage your home and keep
you company. He runs fully autonomously in the background, proactively
checking in on you and alerting you when needed. He speaks aloud via
text-to-speech and supports users with visual, hearing, motor, or other
impairments through a comprehensive accessibility system.

---

## Features

| Category | Capability |
|---|---|
| **Home Control** | Lights (per-room on/off/dimming), thermostat, door locks, security alarm, appliances |
| **Wellness Monitoring** | Scheduled check-ins (morning / midday / evening), wellness score, alert escalation if silent |
| **Learning** | Remember facts, preferences, custom responses, schedules – all persisted across sessions |
| **Question Answering** | Answers from its growing knowledge base, built-in time/date, home-device queries |
| **Autonomous Operation** | Background tick thread – proactively checks in and alerts without waiting for user input; auto-saves every 5 minutes |
| **Voice Output (TTS)** | Speaks responses aloud via espeak-ng / flite / festival / macOS `say` |
| **Blind Mode** | Full verbal descriptions of all information; TTS is enabled automatically |
| **Deaf Mode** | All alerts are visual; prominent text banners; no audio-only cues |
| **DeafBlind Mode** | Screen-reader / Braille-display-friendly linear output; no decorative characters |
| **Simplified Mode** | Short plain-language sentences; reduced jargon – suitable for less-abled users |

---

## Accessibility

Pablo is designed to assist **everyone**, regardless of ability.

### Command-line flags

```bash
./pablo --voice           # Enable text-to-speech output
./pablo --blind           # Full blind-assistance mode (TTS + descriptions)
./pablo --deaf            # Deaf-friendly visual mode (no audio-only cues)
./pablo --simplified      # Simplified language mode
./pablo --screen-reader   # Screen-reader / Braille-friendly mode
./pablo --deafblind       # Combined deaf+blind (all three of the above)
```

### Runtime commands (type at any time)

```
set mode voice            – Toggle TTS on/off
set mode blind            – Enable blind-assistance mode
set mode deaf             – Enable deaf-friendly mode
set mode simplified       – Enable simplified language
set mode screen reader    – Enable screen-reader / Braille mode
set mode deafblind        – Enable combined deaf+blind mode
set mode normal           – Reset to standard text mode
accessibility status      – Show active accessibility settings
accessibility help        – Show full accessibility guide
```

### TTS setup

Pablo auto-detects an installed engine in this priority order:
`espeak-ng` → `espeak` → `flite` → `spd-say` → `festival` → macOS `say`

```bash
# Linux (recommended):
sudo apt install espeak-ng

# Alternative:
sudo apt install flite

# macOS: built-in 'say' requires no install
```

---

## Building

Requirements: **CMake ≥ 3.14**, **C++17** compiler, **pthreads** (standard on Linux/macOS).

```bash
mkdir build && cd build
cmake ..
make -j4          # or: cmake --build .
```

---

## Running

```bash
cd build
./pablo                   # standard text mode
./pablo --voice           # voice output
./pablo --blind           # blind-assistance mode
./pablo --deaf            # deaf-friendly mode
./pablo --deafblind       # combined deaf+blind
./pablo --data=/my/dir    # custom data directory
```

### Example session

```
You: hello
Pablo: Hello! I'm Pablo, your home AI assistant. How can I help you today?

You: turn on the kitchen lights
Pablo: I've turned on the kitchen light.

You: set temperature to 22
Pablo: Thermostat set to 22.0°C (mode: heat).

You: lock the front door
Pablo: I've locked front door.

You: remember that my emergency contact is John at 555-1234
Pablo: I've noted that "my emergency contact" is "john at 555-1234".

You: I prefer jazz music
Pablo: I've saved your preference: music → jazz.

You: when I say goodnight respond Sweet dreams!
Pablo: Understood! When you say "goodnight", I'll respond: "Sweet dreams!".

You: check in on me
Pablo: How are you feeling today?

You: I feel great
Pablo: That's wonderful to hear! I'm glad you're doing well. Wellness score: 80/100.

You: show me what you know
Pablo: === Pablo's Knowledge ===
--- Facts ---
  my emergency contact = john at 555-1234
...

You: goodbye
Pablo: Goodbye! I've saved everything. Take care!
```

### Built-in shell commands

| Command | Effect |
|---|---|
| `help` | Show all capabilities |
| `history` | Print conversation history |
| `home status` | Full smart-home status report |
| `wellness` | Occupant wellness report |
| `knowledge` | Dump all learned knowledge |
| `activity log` | Recent activity log |
| `save` | Force-save the knowledge base |
| `quit` / `exit` | Save and exit |

---

## Tests

```bash
cd build
ctest --output-on-failure
```

Five test suites cover all major subsystems:
- `KnowledgeBase` – storage and persistence
- `NLPEngine` – intent classification and entity extraction
- `HomeManager` – device control
- `LearningSystem` – teach/forget/query
- `PabloIntegration` – end-to-end conversation flows

---

## Architecture

```
pablo/
├── CMakeLists.txt
├── include/
│   ├── knowledge_base.h   # Persistent key-value store
│   ├── nlp_engine.h       # Tokenisation, intent & entity extraction
│   ├── home_manager.h     # Smart-home device simulation
│   ├── occupant_monitor.h # Wellness check-ins and alerts
│   ├── learning_system.h  # Teach/forget/query wrapper
│   └── pablo.h            # Main AI class integrating all modules
├── src/
│   ├── *.cpp              # Implementations
│   └── main.cpp           # Conversation loop entry point
├── data/
│   └── pablo.kb           # Default seed knowledge (plain text)
└── tests/
    └── test_*.cpp         # Unit / integration tests
```

---

## Teaching Pablo

Pablo learns and remembers information across sessions:

```
# Facts
remember that [key] is [value]

# Preferences
I prefer [value] for [key]

# Custom responses
when I say [trigger] respond [your response]

# Schedules / reminders
remind me at 07:00 to take my medication

# Forget something
forget [key]

# Review everything learned
show me what you know
```
