#include "learning_system.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>

// ---- Helpers ----------------------------------------------------------------

std::string LearningSystem::normalise(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (unsigned char c : s) result += static_cast<char>(std::tolower(c));
    auto b = result.find_first_not_of(' ');
    auto e = result.find_last_not_of(' ');
    return (b == std::string::npos) ? "" : result.substr(b, e - b + 1);
}

std::vector<std::string> LearningSystem::tokenise(const std::string& s) {
    std::vector<std::string> tokens;
    std::istringstream ss(s);
    std::string word;
    while (ss >> word) tokens.push_back(word);
    return tokens;
}

std::string LearningSystem::extractAfter(const std::string& input,
                                          const std::string& marker) {
    // case-insensitive search
    std::string lowInput = normalise(input);
    std::string lowMarker = normalise(marker);
    auto pos = lowInput.find(lowMarker);
    if (pos == std::string::npos) return "";
    pos += lowMarker.size();
    // skip spaces
    while (pos < input.size() && input[pos] == ' ') ++pos;
    return input.substr(pos);
}

// ---- Construction -----------------------------------------------------------

LearningSystem::LearningSystem(KnowledgeBase& kb) : kb_(kb) {}

// ---- Main entry: learn ------------------------------------------------------

LearnResult LearningSystem::learn(const std::string& input) {
    std::string norm = normalise(input);
    auto tokens = tokenise(norm);

    auto containsAny = [&](const std::vector<std::string>& kws) {
        for (auto& kw : kws)
            if (std::find(tokens.begin(), tokens.end(), kw) != tokens.end())
                return true;
        return false;
    };

    // Detect learning type
    if (containsAny({"prefer", "like", "love", "favourite", "favorite",
                      "enjoy", "want", "dislike", "hate", "preference"})) {
        return parsePreference(input);
    }
    if (containsAny({"when", "say", "respond", "reply"})) {
        return parseResponse(input);
    }
    if (containsAny({"remind", "schedule", "at", "every", "routine",
                      "morning", "evening", "daily", "alarm"})) {
        if (containsAny({"remind", "schedule", "every", "daily", "routine",
                          "alarm", "morning", "evening"})) {
            return parseSchedule(input);
        }
    }
    // Default: teach a fact
    return parseFact(input);
}

// ---- Main entry: forget -----------------------------------------------------

LearnResult LearningSystem::forget(const std::string& input) {
    return parseForget(input);
}

// ---- Direct stores ----------------------------------------------------------

LearnResult LearningSystem::storePreference(const std::string& key,
                                             const std::string& value) {
    kb_.store("preference", key, value);
    LearnResult r;
    r.success      = true;
    r.type         = LearnType::PREFERENCE;
    r.key          = key;
    r.value        = value;
    r.confirmation = "Got it! I've noted your preference: " + key + " = " + value + ".";
    return r;
}

LearnResult LearningSystem::storeFact(const std::string& key,
                                       const std::string& value) {
    kb_.store("fact", key, value);
    LearnResult r;
    r.success      = true;
    r.type         = LearnType::FACT;
    r.key          = key;
    r.value        = value;
    r.confirmation = "I've noted that " + key + " is " + value + ".";
    return r;
}

LearnResult LearningSystem::storeResponse(const std::string& trigger,
                                           const std::string& response) {
    kb_.store("response", normalise(trigger), response);
    LearnResult r;
    r.success      = true;
    r.type         = LearnType::RESPONSE;
    r.key          = trigger;
    r.value        = response;
    r.confirmation = "Understood! When you say \"" + trigger +
                     "\", I'll respond: \"" + response + "\".";
    return r;
}

LearnResult LearningSystem::storeSchedule(const std::string& timeKey,
                                           const std::string& task) {
    kb_.store("schedule", timeKey, task);
    LearnResult r;
    r.success      = true;
    r.type         = LearnType::SCHEDULE;
    r.key          = timeKey;
    r.value        = task;
    r.confirmation = "Scheduled: I'll remind you to \"" + task +
                     "\" at " + timeKey + ".";
    return r;
}

// ---- Parse fact -------------------------------------------------------------

LearnResult LearningSystem::parseFact(const std::string& input) {
    LearnResult r;
    r.type = LearnType::FACT;

    // Try: "remember that X is Y"  /  "know that X is Y"  /  "X is Y"
    std::string content;
    for (auto& marker : {"remember that", "note that", "learn that",
                          "know that", "did you know that", "store that",
                          "remember", "note", "learn"}) {
        content = extractAfter(input, marker);
        if (!content.empty()) break;
    }
    if (content.empty()) content = input;

    // Split on " is " or " = " or " are "
    for (auto& sep : {" is ", " = ", " are ", " was ", " means "}) {
        auto pos = content.find(sep);
        if (pos != std::string::npos) {
            r.key   = normalise(content.substr(0, pos));
            r.value = normalise(content.substr(pos + std::strlen(sep)));
            break;
        }
    }

    if (r.key.empty() || r.value.empty()) {
        // Store the whole thing as a free-form note
        r.key   = normalise(content).substr(0, 40);
        r.value = normalise(content);
    }

    if (!r.key.empty() && !r.value.empty()) {
        kb_.store("fact", r.key, r.value);
        r.success      = true;
        r.confirmation = "I've noted that \"" + r.key + "\" is \"" + r.value + "\".";
    } else {
        r.confirmation = "I'm not quite sure what to remember. Try: "
                         "\"remember that X is Y\".";
    }
    return r;
}

// ---- Parse preference -------------------------------------------------------

LearnResult LearningSystem::parsePreference(const std::string& input) {
    LearnResult r;
    r.type = LearnType::PREFERENCE;

    std::string content;
    for (auto& marker : {"i prefer", "i like", "i love", "i enjoy",
                          "my preference is", "i want", "my favorite is",
                          "my favourite is", "i dislike", "i hate"}) {
        content = extractAfter(input, marker);
        if (!content.empty()) break;
    }
    if (content.empty()) content = input;

    // "X for Y"  or  "X" (just a value with topic from context)
    auto forPos = content.find(" for ");
    if (forPos != std::string::npos) {
        r.value = normalise(content.substr(0, forPos));
        r.key   = normalise(content.substr(forPos + 5));
    } else {
        // "temperature at 22" / "lights dim" / "music jazz"
        auto tokens = tokenise(content);
        if (tokens.size() >= 2) {
            r.key   = tokens[0];
            r.value = normalise(content.substr(tokens[0].size() + 1));
        } else {
            r.key   = "general";
            r.value = normalise(content);
        }
    }

    if (!r.key.empty() && !r.value.empty()) {
        kb_.store("preference", r.key, r.value);
        r.success      = true;
        r.confirmation = "I've saved your preference: " + r.key + " → " + r.value + ".";
    } else {
        r.confirmation = "I'll remember you have a preference, but could you be "
                         "more specific? Try: \"I prefer jazz music\" or "
                         "\"I prefer 22°C for temperature\".";
    }
    return r;
}

// ---- Parse custom response --------------------------------------------------

LearnResult LearningSystem::parseResponse(const std::string& input) {
    LearnResult r;
    r.type = LearnType::RESPONSE;

    // "when I say X respond Y"  /  "when I ask X answer Y"
    std::string trigger, response;
    auto sayPos = input.find(" say ");
    auto respondPos = input.find(" respond ");
    auto replyPos   = input.find(" reply ");
    auto answerPos  = input.find(" answer ");
    auto sayPos2    = input.find(" ask ");

    std::size_t triggerEnd = std::string::npos;
    std::size_t responseStart = std::string::npos;

    if (sayPos != std::string::npos) {
        triggerEnd = sayPos + 5;
    } else if (sayPos2 != std::string::npos) {
        triggerEnd = sayPos2 + 5;
    }

    if (respondPos != std::string::npos && respondPos > triggerEnd) {
        responseStart = respondPos + 9;
    } else if (replyPos != std::string::npos && replyPos > triggerEnd) {
        responseStart = replyPos + 7;
    } else if (answerPos != std::string::npos && answerPos > triggerEnd) {
        responseStart = answerPos + 8;
    }

    if (triggerEnd != std::string::npos && responseStart != std::string::npos &&
        responseStart > triggerEnd) {
        trigger  = normalise(input.substr(triggerEnd,
                                          responseStart - 9 - triggerEnd));
        response = normalise(input.substr(responseStart));
    }

    if (!trigger.empty() && !response.empty()) {
        return storeResponse(trigger, response);
    }

    r.confirmation = "To teach me a custom response, try: "
                     "\"when I say hello respond Hello there, friend!\"";
    return r;
}

// ---- Parse schedule ---------------------------------------------------------

LearnResult LearningSystem::parseSchedule(const std::string& input) {
    LearnResult r;
    r.type = LearnType::SCHEDULE;

    // Look for time pattern HH:MM or keywords
    std::string timeKey;
    std::string task;

    // Look for HH:MM
    for (std::size_t i = 0; i + 4 < input.size(); ++i) {
        if (std::isdigit(input[i]) && std::isdigit(input[i+1]) &&
            input[i+2] == ':' &&
            std::isdigit(input[i+3]) && std::isdigit(input[i+4])) {
            timeKey = input.substr(i, 5);
            break;
        }
    }

    // Time keywords
    if (timeKey.empty()) {
        std::string norm = normalise(input);
        if (norm.find("morning")   != std::string::npos) timeKey = "morning";
        else if (norm.find("midday") != std::string::npos ||
                 norm.find("noon")   != std::string::npos) timeKey = "midday";
        else if (norm.find("evening") != std::string::npos) timeKey = "evening";
        else if (norm.find("night")   != std::string::npos) timeKey = "night";
    }

    // Task is everything after "to " / "remind me to " / "that I should "
    for (auto& marker : {"remind me to", "to ", "that i should ", "i should "}) {
        task = extractAfter(input, marker);
        if (!task.empty()) break;
    }

    if (timeKey.empty()) timeKey = "unspecified";
    if (task.empty())    task    = normalise(input);

    return storeSchedule(timeKey, task);
}

// ---- Parse forget -----------------------------------------------------------

LearnResult LearningSystem::parseForget(const std::string& input) {
    LearnResult r;
    r.type = LearnType::FORGET;

    std::string content;
    for (auto& marker : {"forget that", "delete", "remove", "unlearn", "erase",
                          "forget"}) {
        content = extractAfter(input, marker);
        if (!content.empty()) break;
    }
    if (content.empty()) content = input;
    content = normalise(content);

    // Try all categories
    bool found = false;
    for (auto& cat : {"fact", "preference", "response", "schedule", "user"}) {
        if (kb_.erase(cat, content)) { found = true; break; }
    }

    if (found) {
        r.success      = true;
        r.key          = content;
        r.confirmation = "I've forgotten \"" + content + "\".";
    } else {
        r.confirmation = "I don't have anything stored as \"" + content + "\".";
    }
    return r;
}

// ---- Query ------------------------------------------------------------------

std::string LearningSystem::findCustomResponse(const std::string& input) const {
    std::string norm = normalise(input);
    std::string val;
    if (kb_.query("response", norm, val)) return val;

    // Fuzzy: search for partial matches
    auto entries = kb_.getByCategory("response");
    for (auto& e : entries) {
        if (norm.find(e.key) != std::string::npos ||
            e.key.find(norm) != std::string::npos) {
            return e.value;
        }
    }
    return "";
}

std::string LearningSystem::getPreference(const std::string& key) const {
    std::string val;
    kb_.query("preference", normalise(key), val);
    return val;
}

std::string LearningSystem::getFact(const std::string& key) const {
    std::string val;
    kb_.query("fact", normalise(key), val);
    return val;
}

std::string LearningSystem::getLearningReport() const {
    std::ostringstream oss;
    oss << "=== Pablo's Knowledge ===\n\n";

    auto printCategory = [&](const std::string& cat, const std::string& title) {
        auto entries = kb_.getByCategory(cat);
        if (!entries.empty()) {
            oss << "--- " << title << " (" << entries.size() << ") ---\n";
            for (auto& e : entries) {
                oss << "  " << e.key << " = " << e.value << "\n";
            }
            oss << "\n";
        }
    };

    printCategory("fact",       "Facts");
    printCategory("preference", "Preferences");
    printCategory("response",   "Custom Responses");
    printCategory("schedule",   "Schedules");
    printCategory("user",       "User Info");

    if (kb_.empty()) oss << "I haven't learned anything yet. Teach me something!\n";
    return oss.str();
}

std::size_t LearningSystem::totalLearned() const { return kb_.size(); }
