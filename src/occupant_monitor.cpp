#include "occupant_monitor.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>

// ---- Helpers ----------------------------------------------------------------

std::string OccupantMonitor::timeString(std::time_t t) {
    if (t == 0) return "never";
    char buf[32];
    std::tm* tm = std::localtime(&t);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm);
    return buf;
}

int OccupantMonitor::scoreFromKeyword(const std::string& response) {
    std::string r = response;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    // Positive signals
    if (r.find("great") != std::string::npos ||
        r.find("excellent") != std::string::npos ||
        r.find("amazing") != std::string::npos ||
        r.find("fantastic") != std::string::npos) return 10;
    if (r.find("good") != std::string::npos ||
        r.find("well") != std::string::npos ||
        r.find("fine") != std::string::npos ||
        r.find("ok") != std::string::npos) return 5;
    // Neutral
    if (r.find("alright") != std::string::npos ||
        r.find("so-so") != std::string::npos ||
        r.find("average") != std::string::npos) return 0;
    // Negative signals
    if (r.find("tired") != std::string::npos ||
        r.find("sleepy") != std::string::npos ||
        r.find("bored") != std::string::npos) return -3;
    if (r.find("bad") != std::string::npos ||
        r.find("unwell") != std::string::npos ||
        r.find("sick") != std::string::npos ||
        r.find("ill") != std::string::npos) return -7;
    if (r.find("pain") != std::string::npos ||
        r.find("hurt") != std::string::npos ||
        r.find("injury") != std::string::npos) return -10;
    if (r.find("emergency") != std::string::npos ||
        r.find("help") != std::string::npos ||
        r.find("danger") != std::string::npos) return -20;
    return 0;
}

// ---- Construction -----------------------------------------------------------

OccupantMonitor::OccupantMonitor() {
    lastActivity_ = std::time(nullptr);
    wellnessScore_ = 70;

    wellnessQuestions_ = {
        "How are you feeling today?",
        "On a scale of 1-10, how's your energy level?",
        "Have you eaten and had enough water today?",
        "How did you sleep last night?",
        "Is there anything I can help you with to make your day better?",
        "Any pain or discomfort I should know about?",
        "Have you taken any medications you need today?",
        "How's your mood today?",
    };

    // Default check-in schedule
    schedule_.push_back({"07:00", "morning"});
    schedule_.push_back({"12:00", "midday"});
    schedule_.push_back({"19:00", "evening"});
}

// ---- Profile ----------------------------------------------------------------

void OccupantMonitor::setProfile(const OccupantProfile& p) { profile_ = p; }
OccupantProfile OccupantMonitor::getProfile() const { return profile_; }
void OccupantMonitor::setName(const std::string& name) { profile_.name = name; }
void OccupantMonitor::setEmergencyContact(const std::string& c) { profile_.emergencyContact = c; }
void OccupantMonitor::setMedicalNotes(const std::string& n) { profile_.medicalNotes = n; }

// ---- Activity ---------------------------------------------------------------

void OccupantMonitor::recordActivity(const std::string& description) {
    lastActivity_ = std::time(nullptr);
    alertLevel_   = AlertLevel::NONE;
    awaitingResponse_ = false;
    if (!description.empty()) addLogEntry(description);
}

// ---- Check-in ---------------------------------------------------------------

std::string OccupantMonitor::initiateCheckIn() {
    awaitingResponse_ = true;
    std::string question = wellnessQuestions_[questionIndex_];
    questionIndex_ = (questionIndex_ + 1) % wellnessQuestions_.size();
    addLogEntry("Check-in initiated.");
    return question;
}

std::string OccupantMonitor::processCheckInResponse(const std::string& response) {
    recordActivity("Check-in response: " + response);
    awaitingResponse_ = false;

    int delta = scoreFromKeyword(response);
    adjustWellnessScore(delta);

    std::ostringstream oss;
    if (delta <= -20) {
        oss << "That sounds serious, " << profile_.name
            << "! I'm very concerned about you. ";
        if (!profile_.emergencyContact.empty()) {
            oss << "I'm flagging this for your emergency contact: "
                << profile_.emergencyContact << ". ";
        }
        oss << "Please call emergency services if you need immediate help (911).";
        alertLevel_ = AlertLevel::EMERGENCY;
    } else if (delta <= -7) {
        oss << "I'm sorry to hear you're not feeling well, " << profile_.name
            << ". Please rest and let me know if you need anything. "
            << "Would you like me to dim the lights and lower the temperature?";
    } else if (delta >= 5) {
        oss << "That's wonderful to hear, " << profile_.name
            << "! I'm glad you're doing well. Wellness score: "
            << wellnessScore_ << "/100.";
    } else {
        oss << "Thank you for checking in, " << profile_.name
            << ". Current wellness score: " << wellnessScore_ << "/100. "
            << "Let me know if there's anything I can do for you.";
    }
    return oss.str();
}

// ---- Alert system -----------------------------------------------------------

AlertLevel OccupantMonitor::getAlertLevel() const {
    if (lastActivity_ == 0) return AlertLevel::NONE;
    std::time_t now = std::time(nullptr);
    double elapsed = std::difftime(now, lastActivity_);

    if (elapsed >= EMERGENCY_THRESHOLD) return AlertLevel::EMERGENCY;
    if (elapsed >= URGENT_THRESHOLD)    return AlertLevel::URGENT;
    if (elapsed >= MODERATE_THRESHOLD)  return AlertLevel::MODERATE;
    if (elapsed >= GENTLE_THRESHOLD)    return AlertLevel::GENTLE;
    return AlertLevel::NONE;
}

std::string OccupantMonitor::getAlertMessage() const {
    AlertLevel al = getAlertLevel();
    switch (al) {
        case AlertLevel::NONE:     return "";
        case AlertLevel::GENTLE:
            return "Hey " + profile_.name + ", just checking in on you. "
                   "How are you doing?";
        case AlertLevel::MODERATE:
            return profile_.name + ", I haven't heard from you in a while. "
                   "Please let me know you're alright.";
        case AlertLevel::URGENT:
            return "I'm quite concerned, " + profile_.name + ". "
                   "It's been many hours since we last spoke. Are you okay?";
        case AlertLevel::EMERGENCY:
            return "EMERGENCY ALERT: I haven't heard from " + profile_.name +
                   " in over 24 hours!" +
                   (profile_.emergencyContact.empty() ? "" :
                    " Emergency contact: " + profile_.emergencyContact);
    }
    return "";
}

void OccupantMonitor::acknowledgeAlert() {
    alertLevel_ = AlertLevel::NONE;
    recordActivity("Alert acknowledged.");
}

// ---- Wellness ---------------------------------------------------------------

int OccupantMonitor::getWellnessScore() const { return wellnessScore_; }

void OccupantMonitor::adjustWellnessScore(int delta) {
    wellnessScore_ = std::max(0, std::min(100, wellnessScore_ + delta));
}

std::string OccupantMonitor::getWellnessSummary() const {
    std::ostringstream oss;
    oss << "Wellness summary for " << profile_.name << ":\n";
    oss << "  Score: " << wellnessScore_ << "/100\n";
    oss << "  Last activity: " << timeString(lastActivity_) << "\n";
    std::string alertMsg = getAlertMessage();
    if (!alertMsg.empty()) oss << "  Alert: " << alertMsg << "\n";
    return oss.str();
}

// ---- Activity log -----------------------------------------------------------

void OccupantMonitor::addLogEntry(const std::string& entry) {
    std::time_t now = std::time(nullptr);
    std::string ts  = timeString(now);
    activityLog_.push_back("[" + ts + "] " + entry);
    // Keep log bounded
    if (activityLog_.size() > 1000) {
        activityLog_.erase(activityLog_.begin(),
                           activityLog_.begin() + 500);
    }
}

std::vector<std::string> OccupantMonitor::getRecentLog(std::size_t n) const {
    if (activityLog_.empty()) return {};
    std::size_t start = activityLog_.size() > n ? activityLog_.size() - n : 0;
    return std::vector<std::string>(activityLog_.begin() + static_cast<long>(start),
                                    activityLog_.end());
}

std::string OccupantMonitor::getLogString(std::size_t n) const {
    auto entries = getRecentLog(n);
    if (entries.empty()) return "No activity recorded yet.";
    std::ostringstream oss;
    for (auto& e : entries) oss << e << "\n";
    return oss.str();
}

// ---- Scheduled check-ins ----------------------------------------------------

void OccupantMonitor::addCheckInTime(const std::string& timeHHMM,
                                      const std::string& label) {
    ScheduledCheckIn sci;
    sci.timeHHMM   = timeHHMM;
    sci.label      = label.empty() ? timeHHMM : label;
    sci.doneToday  = false;
    schedule_.push_back(sci);
}

std::string OccupantMonitor::getScheduledCheckIns() const {
    if (schedule_.empty()) return "No check-in schedule set.";
    std::ostringstream oss;
    oss << "Scheduled check-ins:\n";
    for (auto& s : schedule_) {
        oss << "  " << s.timeHHMM << " – " << s.label << "\n";
    }
    return oss.str();
}

std::string OccupantMonitor::checkScheduledDue() {
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    char buf[6];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", tm->tm_hour, tm->tm_min);
    std::string currentHHMM(buf);

    // Reset "doneToday" at midnight
    if (tm->tm_hour == 0 && tm->tm_min == 0) {
        for (auto& s : schedule_) s.doneToday = false;
    }

    for (auto& s : schedule_) {
        if (!s.doneToday && currentHHMM >= s.timeHHMM) {
            // Within 5 minutes of the scheduled time
            int schedMin = 0;
            try {
                schedMin = std::stoi(s.timeHHMM.substr(0, 2)) * 60 +
                           std::stoi(s.timeHHMM.substr(3, 2));
            } catch (...) {
                // Malformed schedule entry – skip it
                s.doneToday = true; // prevent repeated errors today
                continue;
            }
            int nowMin = tm->tm_hour * 60 + tm->tm_min;
            if (nowMin - schedMin >= 0 && nowMin - schedMin < 5) {
                s.doneToday = true;
                return "Scheduled " + s.label + " check-in: " + initiateCheckIn();
            }
        }
    }
    return "";
}

// ---- Status report ----------------------------------------------------------

std::string OccupantMonitor::getStatusReport() const {
    std::ostringstream oss;
    oss << "=== Occupant Monitor ===\n";
    oss << "Name: " << profile_.name << "\n";
    if (!profile_.emergencyContact.empty())
        oss << "Emergency contact: " << profile_.emergencyContact << "\n";
    oss << "Wellness score: " << wellnessScore_ << "/100\n";
    oss << "Last activity: " << timeString(lastActivity_) << "\n";
    std::string alert = getAlertMessage();
    if (!alert.empty()) oss << "Alert: " << alert << "\n";
    oss << getScheduledCheckIns();
    return oss.str();
}
