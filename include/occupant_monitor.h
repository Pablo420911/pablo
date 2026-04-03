#pragma once
#include <string>
#include <vector>
#include <ctime>
#include <functional>

// ---------------------------------------------------------------------------
// OccupantMonitor – tracks the wellbeing of the house occupant.
//
// Features:
//  • Scheduled check-ins  (morning / midday / evening / custom)
//  • Wellness score (0-100) derived from answers to check-in questions
//  • Activity log  (timestamped entries)
//  • Alert escalation (gentle → urgent → emergency) if occupant is silent
//  • User-profile (name, emergency contact, medical notes)
// ---------------------------------------------------------------------------

struct CheckIn {
    std::time_t scheduledTime{0};
    std::time_t respondedTime{0};
    bool        responded{false};
    int         wellnessScore{50};  // 0-100
    std::string notes;
};

struct OccupantProfile {
    std::string name{"Occupant"};
    std::string emergencyContact;
    std::string medicalNotes;
    int         age{0};
};

enum class AlertLevel {
    NONE,
    GENTLE,   // friendly reminder
    MODERATE, // second reminder
    URGENT,   // strong concern
    EMERGENCY // contact emergency services / contact
};

class OccupantMonitor {
public:
    OccupantMonitor();

    // ----- Profile -----
    void setProfile(const OccupantProfile& p);
    OccupantProfile getProfile() const;
    void setName(const std::string& name);
    void setEmergencyContact(const std::string& contact);
    void setMedicalNotes(const std::string& notes);

    // ----- Check-in management -----
    // Called when Pablo interacts with the occupant – resets alert timer.
    void recordActivity(const std::string& description = "");

    // Perform a wellness check-in; returns question to ask occupant.
    std::string initiateCheckIn();

    // Occupant responds to check-in (e.g. "fine", "tired", "pain").
    // Returns Pablo's response and updates wellness score.
    std::string processCheckInResponse(const std::string& response);

    // Returns true if Pablo is currently awaiting a check-in response.
    bool isAwaitingResponse() const { return awaitingResponse_; }

    // ----- Alert system -----
    // Returns current alert level based on time since last activity.
    AlertLevel getAlertLevel() const;

    // Returns a message appropriate for the current alert level (empty if NONE).
    std::string getAlertMessage() const;

    // Silence an ongoing alert (occupant has responded).
    void acknowledgeAlert();

    // ----- Wellness -----
    int  getWellnessScore() const;
    void adjustWellnessScore(int delta);
    std::string getWellnessSummary() const;

    // ----- Activity log -----
    void addLogEntry(const std::string& entry);
    std::vector<std::string> getRecentLog(std::size_t n = 10) const;
    std::string getLogString(std::size_t n = 10) const;

    // ----- Scheduled check-ins -----
    // Register expected check-in times (24h format, e.g. "07:00").
    void addCheckInTime(const std::string& timeHHMM, const std::string& label = "");
    std::string getScheduledCheckIns() const;

    // Check if a scheduled check-in is due (call periodically / on user input).
    // Returns empty string if nothing is due, or a prompt if a check-in is due.
    std::string checkScheduledDue();

    // ----- Status report -----
    std::string getStatusReport() const;

private:
    OccupantProfile profile_;
    int             wellnessScore_{70};
    std::time_t     lastActivity_{0};
    bool            awaitingResponse_{false};
    AlertLevel      alertLevel_{AlertLevel::NONE};

    std::vector<std::string> activityLog_;
    std::vector<CheckIn>     checkInHistory_;

    struct ScheduledCheckIn {
        std::string timeHHMM;   // "07:00"
        std::string label;      // "morning"
        bool        doneToday{false};
    };
    std::vector<ScheduledCheckIn> schedule_;

    // wellness questions pool
    std::vector<std::string> wellnessQuestions_;
    std::size_t questionIndex_{0};

    // How long (seconds) before escalating alerts
    static constexpr int GENTLE_THRESHOLD   =  4 * 3600; //  4 h
    static constexpr int MODERATE_THRESHOLD =  8 * 3600; //  8 h
    static constexpr int URGENT_THRESHOLD   = 16 * 3600; // 16 h
    static constexpr int EMERGENCY_THRESHOLD= 24 * 3600; // 24 h

    static std::string timeString(std::time_t t);
    static int scoreFromKeyword(const std::string& response);
};
