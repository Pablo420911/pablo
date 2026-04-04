#pragma once
#include "knowledge_base.h"
#include "nlp_engine.h"
#include "home_manager.h"
#include "occupant_monitor.h"
#include "learning_system.h"
#include "accessibility.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>

// ---------------------------------------------------------------------------
// Pablo – the main AI class that integrates all subsystems.
//
// Usage:
//   Pablo pablo;
//   pablo.init("data/");       // load persisted knowledge
//   std::string r = pablo.respond("Hello Pablo");
//   pablo.save();              // persist knowledge before exit
// ---------------------------------------------------------------------------

struct ConversationEntry {
    std::string speaker; // "user" or "pablo"
    std::string text;
    std::time_t timestamp{0};
};

class Pablo {
public:
    Pablo();
    ~Pablo();

    // Initialise – loads knowledge base and sets up subsystems.
    // dataDir is the path to the directory containing .kb and config files.
    void init(const std::string& dataDir = "data");

    // Process a single user utterance and return Pablo's response.
    std::string respond(const std::string& userInput);

    // Persist the knowledge base.
    void save();

    // Accessor to subsystems (for direct control / testing)
    HomeManager&          homeManager()          { return home_; }
    OccupantMonitor&      occupantMonitor()       { return monitor_; }
    LearningSystem&       learningSystem()        { return learner_; }
    KnowledgeBase&        knowledgeBase()         { return kb_; }
    NLPEngine&            nlpEngine()             { return nlp_; }
    AccessibilityManager& accessibilityManager()  { return access_; }

    // Conversation history (last N entries)
    std::vector<ConversationEntry> getHistory(std::size_t n = 20) const;

    // Pretty-print conversation history
    std::string getHistoryString(std::size_t n = 20) const;

    // Get Pablo's name / version string
    static std::string version();

    // Periodic tick – call once per second (or as often as convenient)
    // to trigger scheduled check-ins and wellness prompts.
    // Returns a proactive message if something needs attention, or "".
    std::string tick();

private:
    KnowledgeBase        kb_;
    NLPEngine            nlp_;
    HomeManager          home_;
    OccupantMonitor      monitor_;
    LearningSystem       learner_;
    AccessibilityManager access_;

    std::string dataDir_;
    std::vector<ConversationEntry> history_;

    // Last tick time for alert debouncing
    std::time_t lastTickAlert_{0};

    // ---- Response generators ----
    std::string handleGreeting(const ParsedInput& p);
    std::string handleFarewell(const ParsedInput& p);
    std::string handleQuestion(const ParsedInput& p);
    std::string handleCommand(const ParsedInput& p);
    std::string handleTeachFact(const ParsedInput& p);
    std::string handleTeachResponse(const ParsedInput& p);
    std::string handleTeachPreference(const ParsedInput& p);
    std::string handleTeachSchedule(const ParsedInput& p);
    std::string handleHomeStatus(const ParsedInput& p);
    std::string handleWellnessCheck(const ParsedInput& p);
    std::string handleListLearned(const ParsedInput& p);
    std::string handleForget(const ParsedInput& p);
    std::string handleHelp(const ParsedInput& p);
    std::string handleStatus(const ParsedInput& p);
    std::string handleAffirmation(const ParsedInput& p);
    std::string handleNegation(const ParsedInput& p);
    std::string handleUnknown(const ParsedInput& p);

    // Check for a custom learned response first
    std::string checkCustomResponses(const std::string& input);

    // Add entry to conversation history
    void addHistory(const std::string& speaker, const std::string& text);

    // Built-in factual answers (for common questions)
    std::string builtinAnswer(const ParsedInput& p) const;

    // Greeting phrases pool (cycled)
    static const std::vector<std::string> GREETINGS;
    static const std::vector<std::string> ACKNOWLEDGEMENTS;
    static const std::vector<std::string> UNKNOWNS;
    std::size_t greetingIdx_{0};
    std::size_t unknownIdx_{0};
};
