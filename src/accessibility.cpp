#include "accessibility.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

// ---- Construction ----------------------------------------------------------

AccessibilityManager::AccessibilityManager() {
    // Detect TTS engine eagerly so startup can warn if none is available.
    ttsEngine_ = detectTtsEngine();
}

// ---- Mode management -------------------------------------------------------

void AccessibilityManager::setMode(unsigned flags) {
    mode_ = flags;
    // Blind mode implies voice output.
    if (mode_ & ACCESS_BLIND) mode_ |= ACCESS_VOICE_OUT;
}

void AccessibilityManager::addMode(unsigned flags) {
    mode_ |= flags;
    if (mode_ & ACCESS_BLIND) mode_ |= ACCESS_VOICE_OUT;
}

void AccessibilityManager::removeMode(unsigned flags) {
    mode_ &= ~flags;
}

unsigned AccessibilityManager::getMode() const { return mode_; }

bool AccessibilityManager::hasVoiceOut()    const { return (mode_ & ACCESS_VOICE_OUT)     != 0; }
bool AccessibilityManager::isBlindMode()    const { return (mode_ & ACCESS_BLIND)          != 0; }
bool AccessibilityManager::isDeafMode()     const { return (mode_ & ACCESS_DEAF)           != 0; }
bool AccessibilityManager::isSimplified()   const { return (mode_ & ACCESS_SIMPLIFIED)     != 0; }
bool AccessibilityManager::isScreenReader() const { return (mode_ & ACCESS_SCREEN_READER)  != 0; }
bool AccessibilityManager::isDeafBlind()    const { return isBlindMode() && isDeafMode();   }

// ---- TTS detection ---------------------------------------------------------

std::string AccessibilityManager::detectTtsEngine() {
    // Try each engine in preference order.
    for (const char* cmd : {"espeak-ng", "espeak", "flite", "spd-say", "festival", "say"}) {
        std::string checkCmd = std::string("command -v ") + cmd + " >/dev/null 2>&1";
        if (std::system(checkCmd.c_str()) == 0) {
            return cmd;
        }
    }
    return "";
}

// ---- Shell escaping ---------------------------------------------------------

std::string AccessibilityManager::shellEscape(const std::string& text) {
    // Wrap in single quotes and escape any existing single quotes.
    std::string result = "'";
    for (char c : text) {
        if (c == '\'') result += "'\\''";
        else           result += c;
    }
    result += "'";
    return result;
}

// ---- TTS invocation --------------------------------------------------------

void AccessibilityManager::ttsSpeak(const std::string& text) const {
    if (ttsEngine_.empty()) return;

    // Build TTS command based on engine.
    std::string cmd;
    std::string escaped = shellEscape(text);

    if (ttsEngine_ == "espeak-ng" || ttsEngine_ == "espeak") {
        // --ipa: disable IPA output; -s: speed (130 wpm); run in background
        cmd = ttsEngine_ + " -s 140 " + escaped + " >/dev/null 2>&1 &";
    } else if (ttsEngine_ == "flite") {
        cmd = "flite -t " + escaped + " >/dev/null 2>&1 &";
    } else if (ttsEngine_ == "spd-say") {
        cmd = "spd-say " + escaped + " >/dev/null 2>&1 &";
    } else if (ttsEngine_ == "festival") {
        cmd = "echo " + escaped + " | festival --tts >/dev/null 2>&1 &";
    } else if (ttsEngine_ == "say") {
        // macOS
        cmd = "say " + escaped + " &";
    } else {
        return;
    }

    std::system(cmd.c_str()); // NOLINT – intentional system() for TTS
}

// ---- speak -----------------------------------------------------------------

void AccessibilityManager::speak(const std::string& text) const {
    if (!hasVoiceOut()) return;
    if (ttsEngine_.empty()) {
        // Warn once (in visual output only – TTS obviously can't announce itself)
        static bool warnedOnce = false;
        if (!warnedOnce) {
            std::cout << "[TTS] No TTS engine found. Install espeak-ng: "
                         "sudo apt install espeak-ng\n";
            warnedOnce = true;
        }
        return;
    }
    ttsSpeak(text);
}

// ---- stripBoxChars ----------------------------------------------------------

std::string AccessibilityManager::stripBoxChars(const std::string& text) {
    // Replace Unicode box-drawing characters with plain ASCII equivalents.
    std::string result;
    result.reserve(text.size());
    std::size_t i = 0;
    const unsigned char* u = reinterpret_cast<const unsigned char*>(text.c_str());
    while (i < text.size()) {
        // Box-drawing chars are in the range U+2500..U+257F (UTF-8: E2 94..E2 95)
        if (u[i] == 0xE2 && i + 2 < text.size() &&
            u[i+1] >= 0x94 && u[i+1] <= 0x95) {
            // Replace entire 3-byte sequence with a plain dash or pipe
            unsigned char c1 = u[i+1], c2 = u[i+2];
            if (c1 == 0x94 && (c2 == 0x80 || c2 == 0x81)) {
                result += '-'; // horizontal lines
            } else if (c1 == 0x94 && (c2 == 0x82 || c2 == 0x83)) {
                result += '|'; // vertical lines
            } else if (c1 == 0x95 && c2 >= 0x90) {
                result += '+'; // corners and junctions
            } else {
                result += '+';
            }
            i += 3;
        }
        // Fancy bullet • U+2022 (E2 80 A2)
        else if (u[i] == 0xE2 && i + 2 < text.size() &&
                 u[i+1] == 0x80 && u[i+2] == 0xA2) {
            result += '*';
            i += 3;
        }
        // Fancy arrow → U+2192 (E2 86 92)
        else if (u[i] == 0xE2 && i + 2 < text.size() &&
                 u[i+1] == 0x86 && u[i+2] == 0x92) {
            result += "->";
            i += 3;
        }
        // Em dash – U+2013 (E2 80 93) / U+2014 (E2 80 94)
        else if (u[i] == 0xE2 && i + 2 < text.size() &&
                 u[i+1] == 0x80 && (u[i+2] == 0x93 || u[i+2] == 0x94)) {
            result += " - ";
            i += 3;
        }
        // Regular ASCII or multi-byte non-box character
        else {
            result += text[i];
            ++i;
        }
    }
    return result;
}

// ---- simplify --------------------------------------------------------------

std::string AccessibilityManager::simplify(const std::string& text) {
    // For simplified mode: split into sentences and keep only the first two,
    // unless the full text is already short.
    if (text.size() <= 120) return text;

    std::string result;
    int sentenceCount = 0;
    std::size_t start = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '.' || text[i] == '!' || text[i] == '?') {
            // End of a sentence – include the terminator
            result += text.substr(start, i - start + 1);
            // Advance past the terminator and any trailing whitespace
            start = i + 1;
            while (start < text.size() && (text[start] == ' ' || text[start] == '\t'))
                ++start;
            ++sentenceCount;
            if (sentenceCount >= 2) break;
            // Add a space between sentences
            if (sentenceCount < 2 && start < text.size()) result += ' ';
        }
        // Newlines also act as sentence breaks
        if (text[i] == '\n') {
            if (i > start) {
                result += text.substr(start, i - start);
                result += '\n';
                ++sentenceCount;
                start = i + 1;
                if (sentenceCount >= 3) break;
            } else {
                start = i + 1;
            }
        }
    }
    if (result.empty()) result = text.substr(0, 120) + "...";
    return result;
}

// ---- output ----------------------------------------------------------------

void AccessibilityManager::output(const std::string& speaker,
                                   const std::string& text) const {
    std::string displayText = text;

    // Apply transformations in order:
    if (isScreenReader()) displayText = stripBoxChars(displayText);
    if (isSimplified())   displayText = simplify(displayText);

    // Deaf mode: precede Pablo's responses with a clear visual separator
    if (isDeafMode() && speaker == "Pablo") {
        if (!isScreenReader()) {
            std::cout << "\n┌── PABLO ────────────────────────────────────────┐\n";
            std::cout << "│ ";
        } else {
            std::cout << "\n=== PABLO ===\n";
        }
    }

    // Print the text
    std::cout << speaker << ": " << displayText << "\n";

    if (isDeafMode() && speaker == "Pablo") {
        if (!isScreenReader()) {
            std::cout << "└─────────────────────────────────────────────────┘\n";
        } else {
            std::cout << "=============\n";
        }
    }

    // TTS for blind / voice-out mode (speak Pablo's responses only)
    if (hasVoiceOut() && speaker == "Pablo") {
        // For TTS: strip any special chars so speech sounds natural
        std::string spokenText = stripBoxChars(displayText);
        // Replace bullet/arrow markers
        for (char& c : spokenText) {
            if (c == '*' || c == '|') c = ' ';
        }
        speak(spokenText);
    }

    std::cout.flush();
}

// ---- alert -----------------------------------------------------------------

void AccessibilityManager::alert(const std::string& text) const {
    std::string displayText = isScreenReader() ? stripBoxChars(text) : text;
    if (isSimplified()) displayText = simplify(displayText);

    if (!isScreenReader()) {
        std::cout << "\n╔══ ⚠ ALERT ══════════════════════════════════════╗\n";
        std::cout << "║  " << displayText << "\n";
        std::cout << "╚══════════════════════════════════════════════════╝\n\n";
    } else {
        std::cout << "\n*** ALERT: " << displayText << " ***\n\n";
    }

    // Speak the alert
    if (hasVoiceOut()) {
        speak("Alert: " + stripBoxChars(displayText));
    }

    std::cout.flush();
}

// ---- banner ----------------------------------------------------------------

void AccessibilityManager::banner(const std::string& title) const {
    if (isScreenReader()) {
        std::cout << "\n=== " << title << " ===\n";
    } else {
        std::cout << "\n--- " << title << " ---\n";
    }
    std::cout.flush();
}

// ---- info ------------------------------------------------------------------

void AccessibilityManager::info(const std::string& text) const {
    // info() is used for structured data (home status, logs, etc.).
    // Apply screen-reader formatting but NOT simplification, since the
    // user explicitly requested this data.
    std::string displayText = isScreenReader() ? stripBoxChars(text) : text;
    std::cout << displayText << "\n";
    std::cout.flush();
}

// ---- describeMode ----------------------------------------------------------

std::string AccessibilityManager::describeMode() const {
    if (mode_ == ACCESS_NONE) return "Standard text mode (no accessibility modes active).";

    std::string result = "Active accessibility modes: ";
    bool first = true;
    auto add = [&](const char* name) {
        if (!first) result += ", ";
        result += name;
        first = false;
    };
    if (isBlindMode())    add("Blind (voice output + full descriptions)");
    if (isDeafMode())     add("Deaf (visual alerts, no audio-only cues)");
    if (hasVoiceOut() && !isBlindMode()) add("Voice output (TTS)");
    if (isSimplified())   add("Simplified language");
    if (isScreenReader()) add("Screen reader / Braille-friendly");
    result += ".";

    if (hasVoiceOut()) {
        if (ttsEngine_.empty()) {
            result += "\nTTS engine: None found. Install espeak-ng for voice support.";
        } else {
            result += "\nTTS engine: " + ttsEngine_;
        }
    }
    return result;
}

// ---- helpText --------------------------------------------------------------

std::string AccessibilityManager::helpText() {
    return
        "ACCESSIBILITY HELP\n"
        "==================\n\n"
        "Pablo supports the following accessibility modes:\n\n"
        "  VOICE OUTPUT  – Pablo speaks every response aloud via text-to-speech.\n"
        "  BLIND MODE    – Full verbal descriptions of everything; voice is on.\n"
        "  DEAF MODE     – Visual-only alerts; all info in clear text.\n"
        "  SIMPLIFIED    – Short plain-language sentences; no jargon.\n"
        "  SCREEN READER – Braille-display friendly; no decorative characters.\n\n"
        "COMMANDS (type any of these at any time):\n"
        "  set mode voice            – Toggle text-to-speech on/off\n"
        "  set mode blind            – Full blind-assistance mode\n"
        "  set mode deaf             – Deaf-friendly visual mode\n"
        "  set mode simplified       – Simplified language mode\n"
        "  set mode screen reader    – Braille / screen-reader mode\n"
        "  set mode deafblind        – Combined deaf and blind mode\n"
        "  set mode normal           – Reset to standard text mode\n"
        "  accessibility status      – Show current accessibility settings\n"
        "  accessibility help        – Show this help\n\n"
        "COMMAND-LINE FLAGS (when starting Pablo):\n"
        "  ./pablo --voice           – Start with TTS enabled\n"
        "  ./pablo --blind           – Start in blind mode\n"
        "  ./pablo --deaf            – Start in deaf mode\n"
        "  ./pablo --simplified      – Start in simplified mode\n"
        "  ./pablo --screen-reader   – Start in screen-reader mode\n"
        "  ./pablo --deafblind       – Start in combined deaf+blind mode\n\n"
        "TTS ENGINE SETUP:\n"
        "  Linux:  sudo apt install espeak-ng    (recommended)\n"
        "          sudo apt install flite         (alternative)\n"
        "          sudo apt install festival      (alternative)\n"
        "  macOS:  built-in 'say' command (no install needed)\n\n"
        "Note: Modes can be combined freely (e.g. deaf + simplified).";
}

// ---- parseCommand ----------------------------------------------------------

bool AccessibilityManager::parseCommand(const std::string& input) {
    // Normalise to lowercase, trim
    std::string norm;
    norm.reserve(input.size());
    for (unsigned char c : input) norm += static_cast<char>(std::tolower(c));
    auto b = norm.find_first_not_of(' ');
    auto e = norm.find_last_not_of(' ');
    if (b == std::string::npos) return false;
    norm = norm.substr(b, e - b + 1);

    // "accessibility help" or "help accessibility"
    if (norm == "accessibility help" || norm == "help accessibility" ||
        norm == "access help") {
        std::cout << helpText() << "\n";
        return true;
    }
    // "accessibility status"
    if (norm == "accessibility status" || norm == "access status" ||
        norm == "show accessibility" || norm == "what mode") {
        std::cout << describeMode() << "\n";
        return true;
    }

    // "set mode <name>" or just the mode name
    std::string modePart;
    if (norm.substr(0, 9) == "set mode ") {
        modePart = norm.substr(9);
    } else {
        modePart = norm; // try the whole thing
    }
    // trim modePart
    {
        auto mb = modePart.find_first_not_of(' ');
        auto me = modePart.find_last_not_of(' ');
        if (mb == std::string::npos) return false;
        modePart = modePart.substr(mb, me - mb + 1);
    }

    if (modePart == "voice" || modePart == "tts" || modePart == "voice output" ||
        modePart == "voice out") {
        if (hasVoiceOut()) { removeMode(ACCESS_VOICE_OUT); std::cout << "Voice output disabled.\n"; }
        else               { addMode(ACCESS_VOICE_OUT);    std::cout << "Voice output enabled.\n"; }
        if (ttsEngine_.empty()) std::cout << "Note: No TTS engine found. Install espeak-ng.\n";
        return true;
    }
    if (modePart == "blind" || modePart == "vision impaired" ||
        modePart == "visually impaired" || modePart == "vision") {
        addMode(ACCESS_BLIND);
        std::cout << "Blind mode enabled. I will describe everything verbally.\n";
        return true;
    }
    if (modePart == "deaf" || modePart == "hearing impaired" ||
        modePart == "hearing" || modePart == "hard of hearing") {
        addMode(ACCESS_DEAF);
        std::cout << "Deaf mode enabled. All alerts will be visual.\n";
        return true;
    }
    if (modePart == "deafblind" || modePart == "deaf blind" ||
        modePart == "deaf and blind") {
        addMode(static_cast<unsigned>(ACCESS_BLIND) |
                static_cast<unsigned>(ACCESS_DEAF)  |
                static_cast<unsigned>(ACCESS_SCREEN_READER));
        std::cout << "DeafBlind mode enabled. Screen-reader and Braille-friendly output.\n";
        return true;
    }
    if (modePart == "simplified" || modePart == "simple" ||
        modePart == "easy" || modePart == "plain") {
        addMode(ACCESS_SIMPLIFIED);
        std::cout << "Simplified language mode enabled.\n";
        return true;
    }
    if (modePart == "screen reader" || modePart == "screen-reader" ||
        modePart == "braille" || modePart == "screenreader" ||
        modePart == "screen_reader") {
        addMode(ACCESS_SCREEN_READER);
        std::cout << "Screen-reader / Braille mode enabled.\n";
        return true;
    }
    if (modePart == "normal" || modePart == "default" ||
        modePart == "off" || modePart == "none" || modePart == "reset") {
        setMode(ACCESS_NONE);
        std::cout << "Accessibility modes reset to standard text mode.\n";
        return true;
    }

    return false;
}
