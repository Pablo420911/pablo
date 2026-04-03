# Pablo – Home AI Assistant

Pablo is a **C++ AI assistant** designed to help manage your home and keep
you company. He can answer questions, control smart-home devices, learn new
facts and preferences from you, monitor your wellbeing, and grow smarter the
more you interact with him.

---

## Features

| Category | Capability |
|---|---|
| **Home Control** | Lights (per-room on/off/dimming), thermostat, door locks, security alarm, appliances |
| **Wellness Monitoring** | Scheduled check-ins (morning / midday / evening), wellness score, alert escalation if silent |
| **Learning** | Remember facts, preferences, custom responses, schedules – all persisted across sessions |
| **Question Answering** | Answers from its growing knowledge base, built-in time/date, home-device queries |
| **Conversation** | Natural-language interface; teaches Pablo new responses so he gets smarter over time |

---

## Building

Requirements: **CMake ≥ 3.14** and a **C++17** compiler (GCC / Clang / MSVC).

```bash
# From the repository root
mkdir build && cd build
cmake ..
make -j4          # or: cmake --build .
```

The `pablo` executable and a copy of the `data/` directory are placed in
`build/`.

---

## Running

```bash
cd build
./pablo           # uses ./data/ for the knowledge base
./pablo /path/to/data   # custom data directory
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
