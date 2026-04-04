#include "reminder_manager.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <chrono>

using json = nlohmann::json;

ReminderManager::ReminderManager(const std::string& data_path)
    : m_path(data_path + "/reminders.json")
{
    load();
}

void ReminderManager::load() {
    std::lock_guard<std::mutex> lk(m_mutex);
    std::ifstream f(m_path);
    if (!f.is_open()) return;
    json j;
    try { f >> j; } catch (...) { return; }
    for (auto& obj : j) {
        Reminder r;
        r.id         = obj.value("id", 0);
        r.user_id    = obj.value("user_id", "");
        r.channel_id = obj.value("channel_id", "");
        r.message    = obj.value("message", "");
        r.remind_at  = static_cast<std::time_t>(obj.value("remind_at", 0LL));
        m_reminders.push_back(r);
    }
}

void ReminderManager::save() const {
    json arr = json::array();
    for (auto& r : m_reminders) {
        json obj;
        obj["id"]         = r.id;
        obj["user_id"]    = r.user_id;
        obj["channel_id"] = r.channel_id;
        obj["message"]    = r.message;
        obj["remind_at"]  = static_cast<long long>(r.remind_at);
        arr.push_back(obj);
    }
    std::ofstream f(m_path);
    if (f.is_open()) f << arr.dump(2);
}

int ReminderManager::addReminder(const std::string& user_id,
                                  const std::string& channel_id,
                                  const std::string& message,
                                  int                minutes)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    auto now    = std::chrono::system_clock::now();
    auto fire   = now + std::chrono::minutes(minutes);
    auto fire_t = std::chrono::system_clock::to_time_t(fire);

    int newId = 1;
    for (auto& r : m_reminders) {
        if (r.id >= newId) newId = r.id + 1;
    }
    m_reminders.push_back({newId, user_id, channel_id, message, fire_t});
    save();
    return newId;
}

void ReminderManager::tick(const ReminderCallback& cb) {
    std::vector<Reminder> fired;

    {
        std::lock_guard<std::mutex> lk(m_mutex);
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        std::vector<Reminder> pending;
        for (auto& r : m_reminders) {
            if (r.remind_at <= now) {
                fired.push_back(r);
            } else {
                pending.push_back(r);
            }
        }
        m_reminders = std::move(pending);
        if (!fired.empty()) save();
    }  // lock released before callbacks

    for (auto& r : fired) {
        cb(r);
    }
}

std::vector<Reminder> ReminderManager::getReminders(const std::string& user_id) {
    std::lock_guard<std::mutex> lk(m_mutex);
    std::vector<Reminder> result;
    for (auto& r : m_reminders) {
        if (r.user_id == user_id) result.push_back(r);
    }
    return result;
}
