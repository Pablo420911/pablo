# RoninCraftBot 🗡️

A **lightweight, 24/7 Discord to-do & planning bot** written in C++.  
RoninCraftBot helps users on your Discord server manage tasks and get timed reminders.

---

## Features

| Command | Description |
|---|---|
| `/todo add <task>` | Add a new task to your list |
| `/todo list` | View your full to-do list (pending + completed) |
| `/todo done <id>` | Mark a task as complete ✅ |
| `/todo remove <id>` | Delete a task 🗑️ |
| `/todo clear` | Remove all completed tasks |
| `/remind <minutes> <message>` | Get a DM reminder after N minutes (max 1 week) |
| `/myreminders` | List your pending reminders with time remaining |

**Automatic reminders**: When you come online, the bot DMs you the number of pending tasks so you never forget where you left off.

---

## Requirements

- **C++17** compiler (GCC 9+, Clang 9+, MSVC 2019+)
- **CMake 3.15+**
- Internet access during the first build (CMake fetches DPP and nlohmann/json)

### Runtime dependencies (fetched automatically by CMake)

| Library | Version | Purpose |
|---|---|---|
| [D++ (DPP)](https://dpp.dev) | v10.0.35 | Discord WebSocket & REST client |
| [nlohmann/json](https://github.com/nlohmann/json) | v3.11.3 | JSON persistence |

---

## Build

```bash
git clone https://github.com/Pablo420911/pablo.git
cd pablo
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

The binary is placed at `build/RoninCraftBot`.  
A `build/data/` directory is created automatically for persistent storage.

---

## Running

```bash
export DISCORD_TOKEN="your-bot-token-here"
./build/RoninCraftBot
```

The bot will:
1. Connect to Discord and register all slash commands globally.
2. Start a background thread that checks reminders every 30 seconds.
3. Run indefinitely — manage it with `systemd`, `screen`, or `docker`.

### systemd service (optional)

```ini
[Unit]
Description=RoninCraftBot Discord Bot
After=network.target

[Service]
ExecStart=/opt/ronincraftbot/RoninCraftBot
Restart=always
RestartSec=10
Environment=DISCORD_TOKEN=your-bot-token-here
WorkingDirectory=/opt/ronincraftbot

[Install]
WantedBy=multi-user.target
```

---

## Discord Bot Setup

1. Go to [Discord Developer Portal](https://discord.com/developers/applications) and create a new application.
2. Under **Bot**, enable **Server Members Intent** and **Presence Intent**.
3. Copy the bot token and set the `DISCORD_TOKEN` environment variable.
4. Invite the bot to your server with scopes `bot` + `applications.commands` and permissions **Send Messages**, **Create DMs**.

---

## Project Structure

```
pablo/
├── CMakeLists.txt          # Build system
├── src/
│   ├── main.cpp            # Bot entry point & Discord event handlers
│   ├── todo_manager.h/.cpp # Per-user to-do list with JSON persistence
│   └── reminder_manager.h/.cpp # Timed reminder engine
└── data/                   # Runtime data (created automatically)
    ├── todos.json
    └── reminders.json
```

---

## License

See [LICENSE](LICENSE).

