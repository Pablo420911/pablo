#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// ---------------------------------------------------------------------------
// NLPEngine – lightweight natural-language processing.
//
// Provides:
//  • Tokenisation (word splitting, lowercase, punctuation stripping)
//  • Intent classification (see Intent enum)
//  • Entity extraction (device names, numeric values, time strings, people)
//  • Pattern matching using simple wildcard rules
// ---------------------------------------------------------------------------

enum class Intent {
    UNKNOWN,
    GREETING,       // hello, hi, hey …
    FAREWELL,       // bye, goodbye, see you …
    QUESTION,       // who/what/where/when/why/how …
    COMMAND,        // turn on/off, set, open, close, lock …
    TEACH_FACT,     // "remember that …", "did you know …"
    TEACH_RESPONSE, // "when I say X, respond Y"
    TEACH_PREFERENCE, // "I prefer …", "I like …"
    TEACH_SCHEDULE,   // "every morning at …", "remind me at …"
    HOME_STATUS,    // "what is the status of …", "is the door locked?"
    WELLNESS_CHECK, // "how am I doing?", "check on me"
    LIST_LEARNED,   // "what do you know about …", "show preferences"
    FORGET,         // "forget that …", "delete …"
    HELP,           // "help", "what can you do?"
    STATUS,         // "what time is it?", "what's today?"
    AFFIRMATION,    // yes, ok, sure, correct
    NEGATION,       // no, nope, incorrect
};

struct ParsedInput {
    std::string raw;
    std::string normalised;   // lowercase, trimmed
    std::vector<std::string> tokens;
    Intent intent{Intent::UNKNOWN};
    std::string subject;      // main topic extracted
    std::string object;       // target of a command or learn
    std::string value;        // numeric/time/setting value
    std::vector<std::string> entities; // all found named entities
};

class NLPEngine {
public:
    NLPEngine();

    // Main parse entry point
    ParsedInput parse(const std::string& input) const;

    // Utility: tokenise a string into lowercase words
    static std::vector<std::string> tokenise(const std::string& text);

    // Utility: check if tokens contain any of the supplied keywords
    static bool containsAny(const std::vector<std::string>& tokens,
                            const std::vector<std::string>& keywords);

    // Utility: extract content after a prefix keyword
    // e.g. extractAfter(tokens, "that") → everything after "that"
    static std::string extractAfter(const std::vector<std::string>& tokens,
                                    const std::string& marker);

    // Utility: extract content between two markers (inclusive of neither)
    static std::string extractBetween(const std::vector<std::string>& tokens,
                                      const std::string& start,
                                      const std::string& end);

    // Utility: naive sentence-case reconstruction from tokens
    static std::string join(const std::vector<std::string>& tokens,
                            const std::string& sep = " ");

private:
    void classifyIntent(ParsedInput& p) const;
    void extractEntities(ParsedInput& p) const;

    // Known home devices for entity extraction
    std::vector<std::string> knownDevices_;
};
