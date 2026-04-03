#include "nlp_engine.h"
#include <algorithm>
#include <cctype>
#include <sstream>

// ---- Construction ----------------------------------------------------------

NLPEngine::NLPEngine() {
    knownDevices_ = {
        "light", "lights", "lamp", "lamps",
        "thermostat", "temperature", "heat", "cooling", "ac",
        "door", "lock", "front door", "back door", "garage",
        "alarm", "security",
        "tv", "television", "coffee maker", "coffee_maker", "dishwasher",
        "oven", "microwave", "washer", "dryer",
        "living room", "bedroom", "kitchen", "bathroom", "hallway",
        "office", "garage", "basement", "attic",
    };
}

// ---- Tokenise --------------------------------------------------------------

std::vector<std::string> NLPEngine::tokenise(const std::string& text) {
    std::string cleaned;
    cleaned.reserve(text.size());
    for (unsigned char c : text) {
        if (std::isalnum(c) || c == '\'' || c == '-' || c == ':') {
            cleaned += static_cast<char>(std::tolower(c));
        } else {
            cleaned += ' ';
        }
    }
    std::vector<std::string> tokens;
    std::istringstream ss(cleaned);
    std::string word;
    while (ss >> word) {
        if (!word.empty()) tokens.push_back(word);
    }
    return tokens;
}

// ---- Helpers ---------------------------------------------------------------

bool NLPEngine::containsAny(const std::vector<std::string>& tokens,
                             const std::vector<std::string>& keywords) {
    for (auto& kw : keywords) {
        if (std::find(tokens.begin(), tokens.end(), kw) != tokens.end())
            return true;
    }
    return false;
}

std::string NLPEngine::extractAfter(const std::vector<std::string>& tokens,
                                     const std::string& marker) {
    auto it = std::find(tokens.begin(), tokens.end(), marker);
    if (it == tokens.end() || std::next(it) == tokens.end()) return "";
    std::string result;
    for (auto jt = std::next(it); jt != tokens.end(); ++jt) {
        if (!result.empty()) result += ' ';
        result += *jt;
    }
    return result;
}

std::string NLPEngine::extractBetween(const std::vector<std::string>& tokens,
                                       const std::string& start,
                                       const std::string& end) {
    auto it = std::find(tokens.begin(), tokens.end(), start);
    if (it == tokens.end()) return "";
    ++it;
    std::string result;
    for (; it != tokens.end(); ++it) {
        if (*it == end) break;
        if (!result.empty()) result += ' ';
        result += *it;
    }
    return result;
}

std::string NLPEngine::join(const std::vector<std::string>& tokens,
                             const std::string& sep) {
    std::string result;
    for (auto& t : tokens) {
        if (!result.empty()) result += sep;
        result += t;
    }
    return result;
}

// ---- Intent classification -------------------------------------------------

void NLPEngine::classifyIntent(ParsedInput& p) const {
    auto& t = p.tokens;

    // ---- Teaching / management intents (checked before generic question) ----

    // "forget …" / "delete …" / "remove …" / "unlearn …"
    // Must be checked before TEACH_PREFERENCE to avoid "delete my preference"
    // matching preference detection.
    if (containsAny(t, {"forget", "delete", "remove", "unlearn", "erase"})) {
        p.intent = Intent::FORGET;
        return;
    }

    // "when I say X respond Y"  /  "when I ask X say Y"
    if (containsAny(t, {"when"}) &&
        (containsAny(t, {"say", "respond", "reply", "answer"}))) {
        p.intent = Intent::TEACH_RESPONSE;
        return;
    }

    // "what do you know" / "show me what you learned" / "list preferences"
    // Must be checked BEFORE TEACH_FACT to avoid "show me what you know"
    // being mis-classified as a fact-teaching intent.
    if (containsAny(t, {"list", "show", "display"}) &&
        containsAny(t, {"learned", "know", "memory", "preferences",
                         "facts", "responses", "schedules"})) {
        p.intent = Intent::LIST_LEARNED;
        return;
    }
    if (containsAny(t, {"what"}) &&
        containsAny(t, {"learned", "remember", "memory"})) {
        p.intent = Intent::LIST_LEARNED;
        return;
    }

    // "remember that …" / "did you know that …" / "note that …" / "learn that …"
    // Require an explicit commit keyword together with "that"/"this" to avoid
    // matching casual questions that contain words like "know" or "note".
    if (containsAny(t, {"remember", "memorise", "memorize", "note", "learn",
                         "store"}) &&
        containsAny(t, {"that", "this"})) {
        p.intent = Intent::TEACH_FACT;
        return;
    }
    // Standalone "remember"/"store" without "that" is still a teach-fact
    if (containsAny(t, {"remember", "memorize", "memorise", "store"})) {
        p.intent = Intent::TEACH_FACT;
        return;
    }
    // "I prefer …" / "I like …" / "my preference is …"
    if (containsAny(t, {"prefer", "like", "love", "want", "enjoy", "favourite",
                         "favorite", "preference", "dislike", "hate"})) {
        p.intent = Intent::TEACH_PREFERENCE;
        return;
    }
    // "remind me at …" / "every day …" / "schedule …"
    // Require an explicit scheduling keyword (not just "morning"/"evening")
    // to avoid false positives with greetings like "good morning".
    if (containsAny(t, {"remind", "schedule", "alarm", "every", "daily",
                         "routine", "weekly"})) {
        p.intent = Intent::TEACH_SCHEDULE;
        return;
    }

    // ---- Greetings / farewells ----
    if (containsAny(t, {"hello", "hi", "hey", "howdy", "greetings",
                         "good morning", "morning", "good evening",
                         "good afternoon", "good night", "sup", "yo"})) {
        p.intent = Intent::GREETING;
        return;
    }
    if (containsAny(t, {"bye", "goodbye", "farewell", "see you",
                         "goodnight", "night", "later", "ciao", "adios"})) {
        p.intent = Intent::FAREWELL;
        return;
    }

    // ---- Affirmation / negation ----
    if (t.size() <= 4 &&
        containsAny(t, {"yes", "yep", "yup", "yeah", "sure", "ok", "okay",
                         "correct", "right", "affirmative", "absolutely"})) {
        p.intent = Intent::AFFIRMATION;
        return;
    }
    if (t.size() <= 4 &&
        containsAny(t, {"no", "nope", "nah", "negative", "wrong",
                         "incorrect", "not really"})) {
        p.intent = Intent::NEGATION;
        return;
    }

    // ---- Help ----
    if (containsAny(t, {"help", "commands", "capabilities", "features",
                         "guide"})) {
        p.intent = Intent::HELP;
        return;
    }
    // "what can you do?" pattern
    if (containsAny(t, {"what", "which"}) &&
        containsAny(t, {"can", "do", "capable", "able", "support"})) {
        if (containsAny(t, {"you", "pablo", "your"})) {
            p.intent = Intent::HELP;
            return;
        }
    }

    // ---- Status / time ----
    if (containsAny(t, {"time", "date", "today", "now", "day", "month",
                         "year", "clock"}) && t.size() <= 6) {
        p.intent = Intent::STATUS;
        return;
    }

    // ---- Wellness check ----
    if (containsAny(t, {"how am i", "check on me", "wellness", "health",
                         "feeling", "doing"})) {
        p.intent = Intent::WELLNESS_CHECK;
        return;
    }
    if (containsAny(t, {"check"}) &&
        containsAny(t, {"in", "on", "me", "wellness"})) {
        p.intent = Intent::WELLNESS_CHECK;
        return;
    }

    // ---- Home status ----
    if (containsAny(t, {"status", "state"}) &&
        containsAny(t, {"home", "house", "all", "everything", "light",
                         "lights", "door", "thermostat", "alarm",
                         "temperature", "lock"})) {
        p.intent = Intent::HOME_STATUS;
        return;
    }

    // ---- Commands (home control) ----
    if (containsAny(t, {"turn", "switch", "activate", "deactivate",
                         "enable", "disable", "on", "off",
                         "open", "close", "lock", "unlock",
                         "arm", "disarm", "set", "adjust", "increase",
                         "decrease", "raise", "lower", "dim", "brighten"})) {
        p.intent = Intent::COMMAND;
        return;
    }

    // ---- Questions ----
    if (containsAny(t, {"what", "who", "where", "when", "why", "how",
                         "which", "whose", "is", "are", "was", "were",
                         "do", "does", "did", "can", "could", "would",
                         "should", "have", "has", "tell", "explain"})) {
        p.intent = Intent::QUESTION;
        return;
    }

    p.intent = Intent::UNKNOWN;
}

// ---- Entity extraction -----------------------------------------------------

void NLPEngine::extractEntities(ParsedInput& p) const {
    auto& t = p.tokens;

    // Look for device/room names
    for (auto& dev : knownDevices_) {
        std::vector<std::string> devTokens = tokenise(dev);
        if (devTokens.size() == 1) {
            if (std::find(t.begin(), t.end(), devTokens[0]) != t.end()) {
                p.entities.push_back(dev);
                if (p.subject.empty()) p.subject = dev;
            }
        } else if (devTokens.size() == 2) {
            for (std::size_t i = 0; i + 1 < t.size(); ++i) {
                if (t[i] == devTokens[0] && t[i+1] == devTokens[1]) {
                    p.entities.push_back(dev);
                    if (p.subject.empty()) p.subject = dev;
                }
            }
        }
    }

    // Extract numeric value (temperature, brightness)
    for (auto& tok : t) {
        bool isNum = !tok.empty();
        for (char c : tok) {
            if (!std::isdigit(c) && c != '.' && c != '-') { isNum = false; break; }
        }
        if (isNum && p.value.empty()) {
            p.value = tok;
        }
    }

    // Extract time-like token (HH:MM or words like "morning", "evening")
    for (auto& tok : t) {
        if (tok.size() == 5 && tok[2] == ':' &&
            std::isdigit(tok[0]) && std::isdigit(tok[1]) &&
            std::isdigit(tok[3]) && std::isdigit(tok[4])) {
            p.entities.push_back("time:" + tok);
            if (p.value.empty()) p.value = tok;
        }
    }

    // Extract object (content after "that", "the", "my", command target)
    if (p.object.empty()) {
        for (auto& marker : {"that", "the", "my", "about", "for"}) {
            std::string after = extractAfter(t, marker);
            if (!after.empty()) { p.object = after; break; }
        }
    }
}

// ---- Main parse entry point ------------------------------------------------

ParsedInput NLPEngine::parse(const std::string& input) const {
    ParsedInput p;
    p.raw = input;

    // Normalise
    std::string norm;
    norm.reserve(input.size());
    for (unsigned char c : input) norm += static_cast<char>(std::tolower(c));
    // trim
    auto b = norm.find_first_not_of(" \t\r\n");
    auto e = norm.find_last_not_of(" \t\r\n");
    p.normalised = (b == std::string::npos) ? "" : norm.substr(b, e - b + 1);

    p.tokens = tokenise(p.normalised);
    classifyIntent(p);
    extractEntities(p);

    return p;
}
