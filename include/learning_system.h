#pragma once
#include "knowledge_base.h"
#include <string>
#include <vector>
#include <functional>

// ---------------------------------------------------------------------------
// LearningSystem – teaches Pablo new facts, preferences, responses and
// schedules through natural conversation.
//
// Supported learning types:
//   FACT         – "Pablo, remember that [key] is [value]"
//   PREFERENCE   – "Pablo, I prefer [value] for [key]"
//   RESPONSE     – "Pablo, when I say [trigger], respond [response]"
//   SCHEDULE     – "Pablo, remind me at [time] to [task]"
//   USER_INFO    – personal details about the occupant
// ---------------------------------------------------------------------------

enum class LearnType {
    UNKNOWN,
    FACT,
    PREFERENCE,
    RESPONSE,
    SCHEDULE,
    USER_INFO,
    FORGET,
};

struct LearnResult {
    bool        success{false};
    LearnType   type{LearnType::UNKNOWN};
    std::string key;
    std::string value;
    std::string confirmation;  // message to return to user
};

class LearningSystem {
public:
    explicit LearningSystem(KnowledgeBase& kb);

    // ---- Main entry points ----

    // Parse a "teach" statement and store in the knowledge base.
    // Returns a LearnResult describing what was learned.
    LearnResult learn(const std::string& input);

    // Parse a "forget" statement and remove from the knowledge base.
    LearnResult forget(const std::string& input);

    // Directly store a preference (used by HomeManager / OccupantMonitor).
    LearnResult storePreference(const std::string& key, const std::string& value);

    // Directly store a fact.
    LearnResult storeFact(const std::string& key, const std::string& value);

    // Directly store a custom response trigger.
    LearnResult storeResponse(const std::string& trigger, const std::string& response);

    // Directly store a schedule entry.
    LearnResult storeSchedule(const std::string& timeKey, const std::string& task);

    // ---- Query ----

    // Look up a custom response for a given trigger text.
    // Returns empty string if none found.
    std::string findCustomResponse(const std::string& input) const;

    // Look up a preference value.
    std::string getPreference(const std::string& key) const;

    // Look up a fact.
    std::string getFact(const std::string& key) const;

    // Return a summary of everything Pablo has learned.
    std::string getLearningReport() const;

    // Number of items learned in total
    std::size_t totalLearned() const;

private:
    KnowledgeBase& kb_;

    // Parse helpers
    LearnResult parseFact(const std::string& input);
    LearnResult parsePreference(const std::string& input);
    LearnResult parseResponse(const std::string& input);
    LearnResult parseSchedule(const std::string& input);
    LearnResult parseForget(const std::string& input);

    static std::string normalise(const std::string& s);
    static std::vector<std::string> tokenise(const std::string& s);
    static std::string extractAfter(const std::string& input,
                                    const std::string& marker);
};
