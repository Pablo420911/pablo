#include "pablo.h"
#include <iostream>
#include <string>
#include <csignal>
#include <cstdlib>
#include <algorithm>
#include <cctype>

// ---- Global Pablo instance for signal handler ------------------------------
static Pablo* g_pablo = nullptr;

static void signalHandler(int sig) {
    std::cout << "\n\nPablo: Caught signal " << sig
              << ". Saving knowledge base and exiting...\n";
    if (g_pablo) g_pablo->save();
    std::exit(0);
}

// ---- Utility ---------------------------------------------------------------

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

// ---- Banner ----------------------------------------------------------------

static void printBanner() {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════════════╗\n";
    std::cout << "  ║         Welcome to Pablo – Home AI v1.0          ║\n";
    std::cout << "  ║   Your intelligent home assistant & companion     ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "  Type 'help' to see what I can do.\n";
    std::cout << "  Type 'quit' or 'exit' to leave (I'll save everything).\n";
    std::cout << "  Type 'history' to see the conversation history.\n";
    std::cout << "\n";
}

// ---- Main ------------------------------------------------------------------

int main(int argc, char* argv[]) {
    // Determine data directory
    std::string dataDir = "data";
    if (argc > 1) dataDir = argv[1];

    // Set up Pablo
    Pablo pablo;
    g_pablo = &pablo;

    // Handle Ctrl+C and SIGTERM gracefully
    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    pablo.init(dataDir);

    printBanner();

    // Initial greeting
    std::cout << "Pablo: " << pablo.respond("hello") << "\n\n";

    // ---- Main conversation loop ----
    std::string line;
    while (true) {
        // Print prompt
        std::cout << "You: ";
        std::cout.flush();

        if (!std::getline(std::cin, line)) {
            // EOF (e.g. piped input ended)
            break;
        }

        line = trim(line);
        if (line.empty()) {
            // On empty input, run a tick to check for alerts
            std::string alert = pablo.tick();
            if (!alert.empty()) {
                std::cout << "\nPablo: " << alert << "\n\n";
            }
            continue;
        }

        // Special shell commands
        std::string lower = toLower(line);
        if (lower == "quit" || lower == "exit" || lower == "q") {
            std::cout << "Pablo: " << pablo.respond("goodbye") << "\n";
            break;
        }
        if (lower == "history") {
            std::cout << "\n--- Conversation History ---\n";
            std::cout << pablo.getHistoryString(30);
            std::cout << "----------------------------\n\n";
            continue;
        }
        if (lower == "save") {
            pablo.save();
            std::cout << "Pablo: Knowledge base saved.\n\n";
            continue;
        }
        if (lower == "home status" || lower == "full status") {
            std::cout << "\n" << pablo.homeManager().getFullStatus() << "\n";
            continue;
        }
        if (lower == "wellness" || lower == "wellness report") {
            std::cout << "\n" << pablo.occupantMonitor().getStatusReport() << "\n";
            continue;
        }
        if (lower == "knowledge" || lower == "knowledge base") {
            std::cout << "\n" << pablo.learningSystem().getLearningReport() << "\n";
            continue;
        }
        if (lower == "activity log") {
            std::cout << "\n--- Activity Log ---\n";
            std::cout << pablo.occupantMonitor().getLogString(20);
            std::cout << "--------------------\n\n";
            continue;
        }

        // Run tick for background checks
        std::string alert = pablo.tick();
        if (!alert.empty()) {
            std::cout << "\nPablo: " << alert << "\n";
        }

        // Get Pablo's response
        std::string response = pablo.respond(line);
        std::cout << "Pablo: " << response << "\n\n";
    }

    pablo.save();
    std::cout << "\nPablo: Knowledge saved. Goodbye!\n";
    return 0;
}
