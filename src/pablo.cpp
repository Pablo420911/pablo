#include "pablo.h"
#include <algorithm>
#include <sstream>
#include <ctime>
#include <cstring>
#include <iomanip>
#include <iostream>

// ---- Static data -----------------------------------------------------------

const std::vector<std::string> Pablo::GREETINGS = {
    "Hello! I'm Pablo, your home AI assistant. How can I help you today?",
    "Hi there! Good to hear from you. What can I do for you?",
    "Hey! I'm here and ready to help. What do you need?",
    "Greetings! How's everything going? I'm happy to assist.",
    "Hello! Always great to chat with you. What's on your mind?",
};

const std::vector<std::string> Pablo::ACKNOWLEDGEMENTS = {
    "Got it!",
    "Sure thing!",
    "Of course!",
    "Understood.",
    "No problem!",
};

const std::vector<std::string> Pablo::UNKNOWNS = {
    "I'm not sure I understand. Could you rephrase that?",
    "Hmm, that's a new one for me. Can you say that differently?",
    "I didn't quite catch that. Type 'help' to see what I can do.",
    "I'm still learning! Could you try a different phrasing?",
    "I'm not sure how to respond to that. You can teach me by saying: "
    "\"when I say [phrase] respond [answer]\".",
};

// ---- Version ---------------------------------------------------------------

std::string Pablo::version() {
    return "Pablo AI v1.0 – Home Assistant & Companion";
}

// ---- Construction / destruction --------------------------------------------

Pablo::Pablo() : learner_(kb_) {}
Pablo::~Pablo() {}

// ---- Init ------------------------------------------------------------------

void Pablo::init(const std::string& dataDir) {
    dataDir_ = dataDir;
    std::string kbPath = dataDir_ + "/pablo.kb";
    kb_.load(kbPath);
    // Set the save path even if file didn't exist yet
    // (save() will create it)
}

// ---- Save ------------------------------------------------------------------

void Pablo::save() {
    std::string kbPath = dataDir_ + "/pablo.kb";
    kb_.save(kbPath);
}

// ---- Conversation history --------------------------------------------------

void Pablo::addHistory(const std::string& speaker, const std::string& text) {
    ConversationEntry e;
    e.speaker   = speaker;
    e.text      = text;
    e.timestamp = std::time(nullptr);
    history_.push_back(std::move(e));
    // Keep last 200 entries
    if (history_.size() > 200) {
        history_.erase(history_.begin(), history_.begin() + 100);
    }
}

std::vector<ConversationEntry> Pablo::getHistory(std::size_t n) const {
    if (history_.empty()) return {};
    std::size_t start = history_.size() > n ? history_.size() - n : 0;
    return std::vector<ConversationEntry>(
        history_.begin() + static_cast<long>(start), history_.end());
}

std::string Pablo::getHistoryString(std::size_t n) const {
    auto entries = getHistory(n);
    if (entries.empty()) return "No conversation history yet.";
    std::ostringstream oss;
    for (auto& e : entries) {
        oss << (e.speaker == "pablo" ? "Pablo: " : "You:   ") << e.text << "\n";
    }
    return oss.str();
}

// ---- Tick ------------------------------------------------------------------

std::string Pablo::tick() {
    // Proactive occupant check – throttle to once per 5 minutes
    std::time_t now = std::time(nullptr);
    if (std::difftime(now, lastTickAlert_) < 300) return "";

    // Check scheduled wellness prompts
    std::string scheduled = monitor_.checkScheduledDue();
    if (!scheduled.empty()) {
        lastTickAlert_ = now;
        addHistory("pablo", scheduled);
        return scheduled;
    }

    // Check alert level
    std::string alertMsg = monitor_.getAlertMessage();
    if (!alertMsg.empty()) {
        lastTickAlert_ = now;
        addHistory("pablo", alertMsg);
        return alertMsg;
    }

    return "";
}

// ---- Check custom responses ------------------------------------------------

std::string Pablo::checkCustomResponses(const std::string& input) {
    return learner_.findCustomResponse(input);
}

// ---- Bulitin factual answers -----------------------------------------------

std::string Pablo::builtinAnswer(const ParsedInput& p) const {
    auto& t = p.tokens;
    auto containsAny = [&](const std::vector<std::string>& kws) {
        for (auto& kw : kws)
            if (std::find(t.begin(), t.end(), kw) != t.end()) return true;
        return false;
    };

    // Questions about Pablo itself
    if (containsAny({"you", "your"}) &&
        containsAny({"name", "who", "what"})) {
        return "I'm Pablo, your home AI assistant! I'm here to help manage "
               "your home, keep you company, and make your life easier.";
    }
    if (containsAny({"version", "build"})) {
        return version();
    }
    if (containsAny({"time", "clock"})) {
        std::time_t now = std::time(nullptr);
        std::tm* tm = std::localtime(&now);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%H:%M", tm);
        return std::string("The current time is ") + buf + ".";
    }
    if (containsAny({"date", "today", "day"})) {
        std::time_t now = std::time(nullptr);
        std::tm* tm = std::localtime(&now);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%A, %B %d, %Y", tm);
        return std::string("Today is ") + buf + ".";
    }
    if (containsAny({"weather", "outside", "temperature"})) {
        return "I don't have access to live weather data right now, but you "
               "could check a weather app or website. I can manage indoor "
               "temperature via the thermostat though!";
    }
    if (containsAny({"meaning", "life"})) {
        return "The meaning of life is to help you have a comfortable, happy "
               "home! At least, that's my purpose.";
    }

    // Try knowledge base
    if (!p.subject.empty()) {
        std::string val = learner_.getFact(p.subject);
        if (!val.empty()) return "From what I know: " + p.subject + " is " + val + ".";
        auto results = kb_.search(p.subject);
        if (!results.empty()) {
            return "I found this in my memory: [" + results[0].category + "] " +
                   results[0].key + " = " + results[0].value;
        }
    }

    return "";
}

// ---- Main respond ----------------------------------------------------------

std::string Pablo::respond(const std::string& userInput) {
    if (userInput.empty()) return "";

    // Capture whether we're awaiting a wellness check-in response before
    // recordActivity() resets that flag.
    bool awaitingCheckin = monitor_.isAwaitingResponse();

    // Record that the occupant is active
    monitor_.recordActivity(userInput);
    addHistory("user", userInput);

    // If we were in the middle of a wellness check-in, process the response.
    if (awaitingCheckin) {
        std::string r = monitor_.processCheckInResponse(userInput);
        addHistory("pablo", r);
        return r;
    }

    // Check for custom learned response first
    std::string custom = checkCustomResponses(userInput);
    if (!custom.empty()) {
        addHistory("pablo", custom);
        return custom;
    }

    // Parse the input
    ParsedInput p = nlp_.parse(userInput);

    std::string response;
    switch (p.intent) {
        case Intent::GREETING:           response = handleGreeting(p); break;
        case Intent::FAREWELL:           response = handleFarewell(p); break;
        case Intent::QUESTION:           response = handleQuestion(p); break;
        case Intent::COMMAND:            response = handleCommand(p); break;
        case Intent::TEACH_FACT:         response = handleTeachFact(p); break;
        case Intent::TEACH_RESPONSE:     response = handleTeachResponse(p); break;
        case Intent::TEACH_PREFERENCE:   response = handleTeachPreference(p); break;
        case Intent::TEACH_SCHEDULE:     response = handleTeachSchedule(p); break;
        case Intent::HOME_STATUS:        response = handleHomeStatus(p); break;
        case Intent::WELLNESS_CHECK:     response = handleWellnessCheck(p); break;
        case Intent::LIST_LEARNED:       response = handleListLearned(p); break;
        case Intent::FORGET:             response = handleForget(p); break;
        case Intent::HELP:               response = handleHelp(p); break;
        case Intent::STATUS:             response = handleStatus(p); break;
        case Intent::AFFIRMATION:        response = handleAffirmation(p); break;
        case Intent::NEGATION:           response = handleNegation(p); break;
        default:                         response = handleUnknown(p); break;
    }

    addHistory("pablo", response);
    return response;
}

// ---- Handler implementations -----------------------------------------------

std::string Pablo::handleGreeting(const ParsedInput&) {
    std::string r = GREETINGS[greetingIdx_ % GREETINGS.size()];
    ++greetingIdx_;
    return r;
}

std::string Pablo::handleFarewell(const ParsedInput&) {
    save(); // auto-save on exit
    std::string name = monitor_.getProfile().name;
    return "Goodbye, " + name + "! I've saved everything. "
           "Take care and I'll be here whenever you need me.";
}

std::string Pablo::handleQuestion(const ParsedInput& p) {
    // Check builtin answers first
    std::string builtin = builtinAnswer(p);
    if (!builtin.empty()) return builtin;

    // Try knowledge base search – try subject, then individual meaningful tokens
    auto trySearch = [&](const std::string& query) -> std::vector<KnowledgeBase::Entry> {
        return kb_.search(query);
    };

    std::vector<KnowledgeBase::Entry> results;
    if (!p.subject.empty()) results = trySearch(p.subject);

    // If not found via subject, try each content token (skip question words)
    if (results.empty()) {
        static const std::vector<std::string> stopwords = {
            "what", "who", "where", "when", "why", "how", "which",
            "is", "are", "was", "were", "the", "a", "an", "my", "your",
            "its", "it", "i", "you", "me", "have", "has", "do", "does",
            "did", "can", "could", "would", "should", "be", "been",
        };
        for (auto& tok : p.tokens) {
            if (std::find(stopwords.begin(), stopwords.end(), tok) != stopwords.end())
                continue;
            auto r = trySearch(tok);
            if (!r.empty()) { results = r; break; }
        }
    }

    if (!results.empty()) {
        std::ostringstream oss;
        oss << "Here's what I know:\n";
        for (std::size_t i = 0; i < std::min(results.size(), std::size_t(3)); ++i) {
            oss << "  " << results[i].key << " = " << results[i].value << "\n";
        }
        return oss.str();
    }

    // Home-related questions
    auto& t = p.tokens;
    auto containsAny = [&](const std::vector<std::string>& kws) {
        for (auto& kw : kws)
            if (std::find(t.begin(), t.end(), kw) != t.end()) return true;
        return false;
    };
    if (containsAny({"locked", "lock", "door"})) return home_.getLockStatus();
    if (containsAny({"lights", "light", "lamp"})) return home_.getLightStatus();
    if (containsAny({"temperature", "thermostat", "warm", "cold"}))
        return home_.getThermostatStatus();
    if (containsAny({"alarm", "security"})) return home_.getAlarmStatus();
    if (containsAny({"home", "house", "status"})) return home_.getFullStatus();
    if (containsAny({"wellness", "health", "feeling", "check"}))
        return handleWellnessCheck(p);

    return "I don't have an answer to that yet. You can teach me by saying: "
           "\"remember that [topic] is [answer]\"";
}

std::string Pablo::handleCommand(const ParsedInput& p) {
    auto& t = p.tokens;
    auto& subject = p.subject;
    auto& value   = p.value;

    auto containsAny = [&](const std::vector<std::string>& kws) {
        for (auto& kw : kws)
            if (std::find(t.begin(), t.end(), kw) != t.end()) return true;
        return false;
    };

    // Determine on/off action
    bool turnOn  = containsAny({"on", "activate", "enable", "start"});
    bool turnOff = containsAny({"off", "deactivate", "disable", "stop"});

    // Lock / unlock
    if (containsAny({"lock"}))   return home_.controlLock(subject, true);
    if (containsAny({"unlock"})) return home_.controlLock(subject, false);

    // Arm / disarm alarm
    if (containsAny({"arm"}))    return home_.setAlarmMode("away");
    if (containsAny({"disarm"})) return home_.setAlarmMode("disarmed");

    // Temperature
    if (containsAny({"temperature", "thermostat", "heat", "cool", "ac"})) {
        if (!value.empty()) {
            try { return home_.setTemperature(std::stod(value)); }
            catch (...) {}
        }
        if (containsAny({"increase", "raise", "higher", "warmer"})) {
            return home_.setTemperature(
                [&]{ double cur; return (cur = 21.0) + 2.0; }());
        }
        if (containsAny({"decrease", "lower", "cooler"})) {
            return home_.setTemperature(19.0);
        }
        return home_.getThermostatStatus();
    }

    // Brightness
    if (containsAny({"dim", "brighten"})) {
        int brightness = containsAny({"dim"}) ? 30 : 100;
        if (!value.empty()) { try { brightness = std::stoi(value); } catch (...) {} }
        return home_.setLightBrightness(
            subject.empty() ? "living room" : subject, brightness);
    }

    // Lights
    // For "turn on the kitchen lights", extract the room from all entities
    // (p.subject may be "lights" if that was found first as an entity).
    if (containsAny({"light", "lights", "lamp"})) {
        // Find room name: prefer a room entity over the generic "lights" entity
        static const std::vector<std::string> rooms = {
            "living room", "bedroom", "kitchen", "bathroom",
            "hallway", "office", "garage"
        };
        std::string room;
        for (auto& r : rooms) {
            // Check if room name appears in tokens
            std::vector<std::string> rtoks;
            std::istringstream rss(r);
            std::string rw;
            while (rss >> rw) rtoks.push_back(rw);
            bool found = true;
            for (auto& rw2 : rtoks)
                if (std::find(t.begin(), t.end(), rw2) == t.end()) { found = false; break; }
            if (found) { room = r; break; }
        }
        if (room.empty()) room = subject;

        if (turnOn)  return home_.controlLight(room, true);
        if (turnOff) return home_.controlLight(room, false);
        if (!value.empty()) {
            try { return home_.setLightBrightness(room, std::stoi(value)); }
            catch (...) {}
        }
        return home_.getLightStatus(room);
    }

    // Generic appliance / device
    if (!subject.empty()) {
        if (turnOn)  return home_.controlAppliance(subject, true);
        if (turnOff) return home_.controlAppliance(subject, false);
    }

    // All lights
    if (containsAny({"everything", "all"}) && turnOn)  return home_.controlLight("", true);
    if (containsAny({"everything", "all"}) && turnOff) return home_.controlLight("", false);

    return home_.getFullStatus();
}

std::string Pablo::handleTeachFact(const ParsedInput& p) {
    auto result = learner_.learn(p.raw);
    return result.confirmation;
}

std::string Pablo::handleTeachResponse(const ParsedInput& p) {
    auto result = learner_.learn(p.raw);
    return result.confirmation;
}

std::string Pablo::handleTeachPreference(const ParsedInput& p) {
    auto result = learner_.learn(p.raw);

    // Apply some preferences immediately
    auto& t = p.tokens;
    auto containsAny = [&](const std::vector<std::string>& kws) {
        for (auto& kw : kws)
            if (std::find(t.begin(), t.end(), kw) != t.end()) return true;
        return false;
    };

    if (!result.key.empty()) {
        if (containsAny({"temperature", "warm", "cold", "cool", "heat"})) {
            try {
                double temp = std::stod(result.value);
                home_.setTemperature(temp);
            } catch (...) {}
        }
        if (containsAny({"name", "called", "call"})) {
            monitor_.setName(result.value);
        }
    }
    return result.confirmation;
}

std::string Pablo::handleTeachSchedule(const ParsedInput& p) {
    auto result = learner_.learn(p.raw);
    if (result.success && !result.key.empty()) {
        monitor_.addCheckInTime(result.key, result.value);
    }
    return result.confirmation;
}

std::string Pablo::handleHomeStatus(const ParsedInput& p) {
    auto& t = p.tokens;
    auto containsAny = [&](const std::vector<std::string>& kws) {
        for (auto& kw : kws)
            if (std::find(t.begin(), t.end(), kw) != t.end()) return true;
        return false;
    };

    if (containsAny({"light", "lights"})) return home_.getLightStatus();
    if (containsAny({"temperature", "thermostat"})) return home_.getThermostatStatus();
    if (containsAny({"lock", "door", "doors"})) return home_.getLockStatus();
    if (containsAny({"alarm", "security"})) return home_.getAlarmStatus();
    if (containsAny({"appliance", "appliances"})) return home_.getApplianceStatus();
    if (containsAny({"sensor", "sensors"})) return home_.getSensorReadings();
    return home_.getFullStatus();
}

std::string Pablo::handleWellnessCheck(const ParsedInput&) {
    std::string question = monitor_.initiateCheckIn();
    return question;
}

std::string Pablo::handleListLearned(const ParsedInput& p) {
    auto& t = p.tokens;
    auto containsAny = [&](const std::vector<std::string>& kws) {
        for (auto& kw : kws)
            if (std::find(t.begin(), t.end(), kw) != t.end()) return true;
        return false;
    };

    if (containsAny({"preferences", "preference"})) {
        auto entries = kb_.getByCategory("preference");
        if (entries.empty()) return "I haven't learned any preferences yet.";
        std::ostringstream oss;
        oss << "Your preferences:\n";
        for (auto& e : entries) oss << "  " << e.key << " → " << e.value << "\n";
        return oss.str();
    }
    if (containsAny({"schedules", "schedule"})) {
        auto entries = kb_.getByCategory("schedule");
        if (entries.empty()) return "No schedules stored yet.";
        std::ostringstream oss;
        oss << "Scheduled tasks:\n";
        for (auto& e : entries) oss << "  " << e.key << ": " << e.value << "\n";
        return oss.str();
    }
    return learner_.getLearningReport();
}

std::string Pablo::handleForget(const ParsedInput& p) {
    auto result = learner_.forget(p.raw);
    return result.confirmation;
}

std::string Pablo::handleHelp(const ParsedInput&) {
    return
        "Hi! I'm Pablo, your home AI assistant. Here's what I can do:\n\n"
        "HOME CONTROL:\n"
        "  • \"Turn on the kitchen lights\"\n"
        "  • \"Set temperature to 22\"\n"
        "  • \"Lock the front door\"\n"
        "  • \"Arm the alarm\"\n"
        "  • \"Turn on the coffee maker\"\n"
        "  • \"What is the home status?\"\n\n"
        "WELLNESS & CHECK-INS:\n"
        "  • \"Check in on me\"\n"
        "  • \"How am I doing?\"\n"
        "  • \"Remind me at 07:00 to take medication\"\n\n"
        "LEARNING:\n"
        "  • \"Remember that my cat is named Whiskers\"\n"
        "  • \"I prefer 21°C for temperature\"\n"
        "  • \"When I say goodnight, respond Sleep well!\"\n"
        "  • \"Forget that my cat is named Whiskers\"\n"
        "  • \"Show me what you know\"\n\n"
        "GENERAL:\n"
        "  • \"What time is it?\"\n"
        "  • \"What is today's date?\"\n"
        "  • \"Who are you?\"\n"
        "  • \"History\" – show recent conversation\n\n"
        "You can also teach me new facts and I'll remember them across sessions!";
}

std::string Pablo::handleStatus(const ParsedInput& p) {
    auto& t = p.tokens;
    auto containsAny = [&](const std::vector<std::string>& kws) {
        for (auto& kw : kws)
            if (std::find(t.begin(), t.end(), kw) != t.end()) return true;
        return false;
    };

    if (containsAny({"time", "clock"})) {
        std::time_t now = std::time(nullptr);
        std::tm* tm = std::localtime(&now);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", tm);
        return std::string("The current time is ") + buf + ".";
    }
    if (containsAny({"date", "today", "day", "month", "year"})) {
        std::time_t now = std::time(nullptr);
        std::tm* tm = std::localtime(&now);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%A, %B %d, %Y", tm);
        return std::string("Today is ") + buf + ".";
    }
    if (containsAny({"pablo", "you", "your"})) {
        return version() + "\nI've learned " +
               std::to_string(learner_.totalLearned()) + " items so far.";
    }
    // Combined status
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    char tbuf[32], dbuf[64];
    std::strftime(tbuf, sizeof(tbuf), "%H:%M", tm);
    std::strftime(dbuf, sizeof(dbuf), "%A, %B %d, %Y", tm);
    return std::string("Time: ") + tbuf + "\nDate: " + dbuf + "\n" +
           "Wellness score: " + std::to_string(monitor_.getWellnessScore()) + "/100\n" +
           "Items I've learned: " + std::to_string(learner_.totalLearned());
}

std::string Pablo::handleAffirmation(const ParsedInput&) {
    return ACKNOWLEDGEMENTS[greetingIdx_ % ACKNOWLEDGEMENTS.size()];
}

std::string Pablo::handleNegation(const ParsedInput&) {
    return "No problem! Let me know if there's anything else I can do for you.";
}

std::string Pablo::handleUnknown(const ParsedInput& p) {
    // One more attempt: search the knowledge base
    if (!p.normalised.empty()) {
        auto results = kb_.search(p.normalised);
        if (!results.empty()) {
            return "From my memory: " + results[0].key + " = " + results[0].value;
        }
    }
    std::string r = UNKNOWNS[unknownIdx_ % UNKNOWNS.size()];
    ++unknownIdx_;
    return r;
}
