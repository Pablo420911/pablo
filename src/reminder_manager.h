#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include <ctime>

struct Reminder {
    int         id;
    std::string user_id;
    std::string channel_id;   // empty string = send as DM
    std::string message;
    std::time_t remind_at;    // Unix timestamp (UTC)
};

// Callback invoked when a reminder fires.
// Arguments: (user_id, channel_id, message)
using ReminderCallback = std::function<void(const Reminder&)>;

class ReminderManager {
public:
    explicit ReminderManager(const std::string& data_path);

    // schedule a reminder `minutes` from now; returns assigned id
    int addReminder(const std::string& user_id,
                    const std::string& channel_id,
                    const std::string& message,
                    int                minutes);

    // Call this periodically (e.g. every 30 s) to fire due reminders
    void tick(const ReminderCallback& cb);

    std::vector<Reminder> getReminders(const std::string& user_id);

private:
    void load();
    void save() const;

    std::string m_path;
    mutable std::mutex m_mutex;
    std::vector<Reminder> m_reminders;
};
