#include "pablo.h"
#include "accessibility.h"
#include <iostream>
#include <string>
#include <sstream>
#include <csignal>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <condition_variable>

// POSIX select() for non-blocking stdin reads
#include <sys/select.h>
#include <unistd.h>

// ============================================================================
// Globals / shared state
// ============================================================================

static Pablo*    g_pablo    = nullptr;
static std::atomic<bool> g_running{true};

// Background messages from tick thread -> main thread
static std::queue<std::string>   g_bgQueue;
static std::mutex                g_bgMutex;
static std::condition_variable   g_bgCv;

// Mutex protecting all console output
static std::mutex g_printMutex;

// ============================================================================
// Utilities
// ============================================================================

static std::string trim(const std::string& s) {
    auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

// Print an alert through the accessibility layer (thread-safe)
static void pabloAlert(const std::string& text) {
    std::lock_guard<std::mutex> lk(g_printMutex);
    g_pablo->accessibilityManager().alert(text);
    std::cout << "You: ";
    std::cout.flush();
}

// ============================================================================
// Signal handler
// ============================================================================

static void signalHandler(int /*sig*/) {
    g_running = false;
    g_bgCv.notify_all();
}

// ============================================================================
// Background autonomous tick thread
//
// Runs every second, calling pablo.tick() for proactive check-ins / alerts.
// Auto-saves the knowledge base every 5 minutes.
// ============================================================================

static void tickThread() {
    auto nextSave = std::chrono::steady_clock::now() + std::chrono::minutes(5);

    while (g_running.load()) {
        {
            std::unique_lock<std::mutex> lk(g_bgMutex);
            g_bgCv.wait_for(lk, std::chrono::seconds(1),
                            []{ return !g_running.load(); });
        }
        if (!g_running.load()) break;

        std::string msg = g_pablo->tick();
        if (!msg.empty()) {
            std::lock_guard<std::mutex> lk(g_bgMutex);
            g_bgQueue.push(msg);
        }

        auto now = std::chrono::steady_clock::now();
        if (now >= nextSave) {
            g_pablo->save();
            nextSave = now + std::chrono::minutes(5);
        }
    }
}

// ============================================================================
// Drain the background message queue (called from main thread)
// ============================================================================

static void drainBgQueue() {
    std::queue<std::string> local;
    {
        std::lock_guard<std::mutex> lk(g_bgMutex);
        std::swap(local, g_bgQueue);
    }
    while (!local.empty()) {
        pabloAlert(local.front());
        local.pop();
    }
}

// ============================================================================
// Non-blocking stdin check (returns true if data is ready to read)
// ============================================================================

static bool stdinReady(int timeoutMs = 100) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv;
    tv.tv_sec  = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;
}

// ============================================================================
// Banner
// ============================================================================

static void printBanner(const AccessibilityManager& access) {
    if (access.isScreenReader()) {
        std::cout << "\n=== Welcome to Pablo Home AI v1.1 ===\n";
        std::cout << "Your intelligent home assistant and companion.\n";
        std::cout << "Type help for commands. Type accessibility help for options.\n";
        std::cout << "Type quit to exit.\n\n";
    } else {
        std::cout << "\n";
        std::cout << "  ╔══════════════════════════════════════════════════╗\n";
        std::cout << "  ║       Welcome to Pablo -- Home AI v1.1           ║\n";
        std::cout << "  ║   Intelligent assistant for everyone             ║\n";
        std::cout << "  ╚══════════════════════════════════════════════════╝\n";
        std::cout << "\n";
        std::cout << "  Type 'help' for commands.\n";
        std::cout << "  Type 'accessibility help' for accessibility options.\n";
        std::cout << "  Type 'quit' or 'exit' to leave.\n";
        std::cout << "\n";
    }

    if (access.getMode() != ACCESS_NONE) {
        std::cout << "  Accessibility: " << access.describeMode() << "\n\n";
    }
}

// ============================================================================
// Parse CLI flags and configure accessibility mode
// ============================================================================

static void parseCLIFlags(int argc, char* argv[],
                           std::string& dataDir,
                           AccessibilityManager& access) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--voice" || arg == "-v") {
            access.addMode(ACCESS_VOICE_OUT);
        } else if (arg == "--blind") {
            access.addMode(ACCESS_BLIND);
        } else if (arg == "--deaf") {
            access.addMode(ACCESS_DEAF);
        } else if (arg == "--simplified" || arg == "--simple") {
            access.addMode(ACCESS_SIMPLIFIED);
        } else if (arg == "--screen-reader" || arg == "--braille") {
            access.addMode(ACCESS_SCREEN_READER);
        } else if (arg == "--deafblind" || arg == "--deaf-blind") {
            access.addMode(static_cast<unsigned>(ACCESS_BLIND)        |
                           static_cast<unsigned>(ACCESS_DEAF)         |
                           static_cast<unsigned>(ACCESS_SCREEN_READER));
        } else if (arg.size() > 7 && arg.substr(0, 7) == "--data=") {
            dataDir = arg.substr(7);
        } else if (!arg.empty() && arg[0] != '-') {
            dataDir = arg;
        }
    }
}

// ============================================================================
// Handle a built-in shell command
// Returns true if the command was handled, false otherwise.
// ============================================================================

static bool handleBuiltin(const std::string& lower,
                           const std::string& original,
                           Pablo& pablo) {
    auto& access = pablo.accessibilityManager();

    // Accessibility commands take priority
    if (access.parseCommand(lower) || access.parseCommand(original)) {
        return true;
    }

    if (lower == "history") {
        std::lock_guard<std::mutex> lk(g_printMutex);
        access.banner("Conversation History");
        access.info(pablo.getHistoryString(30));
        access.banner("End");
        return true;
    }
    if (lower == "save") {
        pablo.save();
        std::lock_guard<std::mutex> lk(g_printMutex);
        access.info("Knowledge base saved.");
        return true;
    }
    if (lower == "home status" || lower == "full status") {
        std::lock_guard<std::mutex> lk(g_printMutex);
        access.info(pablo.homeManager().getFullStatus());
        return true;
    }
    if (lower == "wellness" || lower == "wellness report") {
        std::lock_guard<std::mutex> lk(g_printMutex);
        access.info(pablo.occupantMonitor().getStatusReport());
        return true;
    }
    if (lower == "knowledge" || lower == "knowledge base") {
        std::lock_guard<std::mutex> lk(g_printMutex);
        access.info(pablo.learningSystem().getLearningReport());
        return true;
    }
    if (lower == "activity log") {
        std::lock_guard<std::mutex> lk(g_printMutex);
        access.banner("Activity Log (last 20 entries)");
        access.info(pablo.occupantMonitor().getLogString(20));
        access.banner("End");
        return true;
    }
    return false;
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    // ---- Parse CLI flags ----
    std::string dataDir = "data";
    AccessibilityManager cliAccess;
    parseCLIFlags(argc, argv, dataDir, cliAccess);

    // ---- Set up Pablo ----
    Pablo pablo;
    pablo.accessibilityManager().setMode(cliAccess.getMode());
    g_pablo = &pablo;

    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    pablo.init(dataDir);

    // ---- Banner ----
    {
        std::lock_guard<std::mutex> lk(g_printMutex);
        printBanner(pablo.accessibilityManager());
    }

    // ---- Initial greeting ----
    {
        std::string greeting = pablo.respond("hello");
        std::lock_guard<std::mutex> lk(g_printMutex);
        pablo.accessibilityManager().output("Pablo", greeting);
        std::cout << "\n";
    }

    // ---- TTS status ----
    if (pablo.accessibilityManager().hasVoiceOut()) {
        std::string engine = AccessibilityManager::detectTtsEngine();
        std::lock_guard<std::mutex> lk(g_printMutex);
        if (engine.empty()) {
            pablo.accessibilityManager().info(
                "Note: Voice output requested but no TTS engine found.\n"
                "      Install espeak-ng:  sudo apt install espeak-ng");
        } else {
            pablo.accessibilityManager().info(
                "Voice output active using: " + engine);
        }
        std::cout << "\n";
    }

    // ---- Start background autonomous tick thread ----
    std::thread ticker(tickThread);

    // ---- Main conversation loop ----
    {
        std::lock_guard<std::mutex> lk(g_printMutex);
        std::cout << "You: ";
        std::cout.flush();
    }

    std::string line;

    while (g_running.load()) {
        // Drain any proactive background messages (alerts, check-ins)
        drainBgQueue();

        // Non-blocking check for input (100ms poll interval)
        if (!stdinReady(100)) continue;

        if (!std::getline(std::cin, line)) {
            // EOF
            break;
        }

        line = trim(line);
        if (line.empty()) {
            std::lock_guard<std::mutex> lk(g_printMutex);
            std::cout << "You: ";
            std::cout.flush();
            continue;
        }

        std::string lower = toLower(line);

        // Exit
        if (lower == "quit" || lower == "exit" || lower == "q") {
            std::string bye = pablo.respond("goodbye");
            std::lock_guard<std::mutex> lk(g_printMutex);
            pablo.accessibilityManager().output("Pablo", bye);
            std::cout << "\n";
            break;
        }

        // Built-in commands
        if (handleBuiltin(lower, line, pablo)) {
            std::lock_guard<std::mutex> lk(g_printMutex);
            std::cout << "\nYou: ";
            std::cout.flush();
            continue;
        }

        // Process with Pablo AI
        std::string response = pablo.respond(line);
        {
            std::lock_guard<std::mutex> lk(g_printMutex);
            pablo.accessibilityManager().output("Pablo", response);
            std::cout << "\nYou: ";
            std::cout.flush();
        }
    }

    // ---- Shutdown ----
    g_running = false;
    g_bgCv.notify_all();
    if (ticker.joinable()) ticker.join();

    pablo.save();
    {
        std::lock_guard<std::mutex> lk(g_printMutex);
        pablo.accessibilityManager().info("\nKnowledge saved. Goodbye!");
    }
    return 0;
}
