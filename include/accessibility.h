#pragma once
#include <string>
#include <atomic>

// ---------------------------------------------------------------------------
// AccessibilityManager – makes Pablo usable by people with a wide range of
// needs, including visual, auditory and motor impairments.
//
// Supported modes (bitfield – any combination is valid):
//
//   ACCESS_VOICE_OUT    – Text-to-speech output via espeak-ng / flite / say
//   ACCESS_BLIND        – Fully verbal mode: every response is spoken, all
//                         visual-only information is described in words too.
//                         (Implies VOICE_OUT automatically.)
//   ACCESS_DEAF         – Visual-only alerts; removes any assumption that
//                         audio will be heard; uses clear text separators.
//   ACCESS_SIMPLIFIED   – Short plain-language sentences; avoids jargon.
//   ACCESS_SCREEN_READER– Braille-display-friendly: no box-drawing characters,
//                         linear predictable output, limited line width.
//
// Runtime commands (type in the conversation loop):
//   accessibility help        – show this information
//   set mode voice            – toggle TTS on/off
//   set mode blind            – enable full blind-assistance mode
//   set mode deaf             – enable deaf-friendly visual mode
//   set mode simplified       – enable simplified language
//   set mode screen reader    – enable screen-reader / Braille mode
//   set mode normal           – reset to default text mode
//   accessibility status      – show current mode
//
// TTS setup (one of the following must be installed):
//   Linux:   sudo apt install espeak-ng
//            sudo apt install flite
//            sudo apt install festival
//            sudo apt install speech-dispatcher  (then spd-say)
//   macOS:   built-in 'say' command (no install needed)
//   Windows: not supported via command-line (use NVDA or similar)
// ---------------------------------------------------------------------------

// Accessibility mode bit-flags
enum AccessMode : unsigned {
    ACCESS_NONE         = 0,
    ACCESS_VOICE_OUT    = 1u << 0,  // TTS spoken output
    ACCESS_BLIND        = 1u << 1,  // comprehensive verbal descriptions (sets VOICE_OUT)
    ACCESS_DEAF         = 1u << 2,  // visual-only alerts, no audio-only cues
    ACCESS_SIMPLIFIED   = 1u << 3,  // short plain-language responses
    ACCESS_SCREEN_READER= 1u << 4,  // minimal decoration, braille-friendly
};

class AccessibilityManager {
public:
    AccessibilityManager();

    // ---- Mode management ----
    void     setMode(unsigned flags);
    void     addMode(unsigned flags);
    void     removeMode(unsigned flags);
    unsigned getMode() const;

    bool hasVoiceOut()   const;
    bool isBlindMode()   const;
    bool isDeafMode()    const;
    bool isSimplified()  const;
    bool isScreenReader() const;
    bool isDeafBlind()   const;   // blind AND deaf simultaneously

    // ---- Output primitives ----

    // Print a response line and optionally speak it via TTS.
    // speaker = "Pablo" or "You" etc.
    void output(const std::string& speaker,
                const std::string& text) const;

    // Print an important alert with visual and/or auditory emphasis.
    void alert(const std::string& text) const;

    // Print a section separator / banner.
    void banner(const std::string& title) const;

    // Print a plain informational line (no speaker prefix).
    void info(const std::string& text) const;

    // Speak text via TTS (no-op if TTS not available / not enabled).
    void speak(const std::string& text) const;

    // ---- Utilities ----

    // Return the name of the first available TTS engine, or "" if none.
    static std::string detectTtsEngine();

    // Shorten / simplify a response string for simplified mode.
    static std::string simplify(const std::string& text);

    // Strip box-drawing characters for screen-reader mode.
    static std::string stripBoxChars(const std::string& text);

    // Human-readable description of currently active modes.
    std::string describeMode() const;

    // Help text explaining accessibility commands.
    static std::string helpText();

    // Try to parse an "accessibility …" or "set mode …" command.
    // Returns true and sets the mode if recognised; returns false otherwise.
    bool parseCommand(const std::string& input);

private:
    unsigned mode_{ACCESS_NONE};
    mutable std::string ttsEngine_; // lazily detected

    // Internal: run TTS in a detached child process.
    void ttsSpeak(const std::string& text) const;

    // Escape text for safe shell quoting (for TTS command).
    static std::string shellEscape(const std::string& text);
};
