#include "todo_manager.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <unordered_map>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helper: current UTC time as ISO-8601 string (thread-safe via gmtime_r)
// ---------------------------------------------------------------------------
static std::string utcNow() {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    struct tm buf{};
    gmtime_r(&t, &buf);
    std::ostringstream ss;
    ss << std::put_time(&buf, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

// ---------------------------------------------------------------------------
TodoManager::TodoManager(const std::string& data_path)
    : m_path(data_path + "/todos.json")
{
    load();
}

void TodoManager::load() {
    std::lock_guard<std::mutex> lk(m_mutex);
    std::ifstream f(m_path);
    if (!f.is_open()) return;

    json j;
    try {
        f >> j;
    } catch (...) {
        return;
    }

    for (auto& [uid, arr] : j.items()) {
        std::vector<TodoItem> items;
        for (auto& obj : arr) {
            TodoItem item;
            item.id         = obj.value("id", 0);
            item.task       = obj.value("task", "");
            item.done       = obj.value("done", false);
            item.created_at = obj.value("created_at", "");
            items.push_back(std::move(item));
        }
        m_todos[uid] = std::move(items);
    }
}

void TodoManager::save() const {
    json j = json::object();
    for (auto& [uid, items] : m_todos) {
        json arr = json::array();
        for (auto& item : items) {
            json obj;
            obj["id"]         = item.id;
            obj["task"]       = item.task;
            obj["done"]       = item.done;
            obj["created_at"] = item.created_at;
            arr.push_back(obj);
        }
        j[uid] = arr;
    }
    std::ofstream f(m_path);
    if (f.is_open()) f << j.dump(2);
}

// ---------------------------------------------------------------------------
int TodoManager::addTodo(const std::string& user_id, const std::string& task) {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto& list  = m_todos[user_id];
    int   newId = 1;
    for (auto& item : list) {
        if (item.id >= newId) newId = item.id + 1;
    }
    list.push_back({newId, task, false, utcNow()});
    save();
    return newId;
}

bool TodoManager::markDone(const std::string& user_id, int id) {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_todos.find(user_id);
    if (it == m_todos.end()) return false;
    for (auto& item : it->second) {
        if (item.id == id) {
            item.done = true;
            save();
            return true;
        }
    }
    return false;
}

void TodoManager::clearDone(const std::string& user_id) {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_todos.find(user_id);
    if (it == m_todos.end()) return;
    auto& list = it->second;
    list.erase(
        std::remove_if(list.begin(), list.end(),
                       [](const TodoItem& i){ return i.done; }),
        list.end()
    );
    save();
}

bool TodoManager::removeTodo(const std::string& user_id, int id) {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_todos.find(user_id);
    if (it == m_todos.end()) return false;
    auto& list = it->second;
    auto  pos  = std::find_if(list.begin(), list.end(),
                               [id](const TodoItem& i){ return i.id == id; });
    if (pos == list.end()) return false;
    list.erase(pos);
    save();
    return true;
}

std::vector<TodoItem> TodoManager::getTodos(const std::string& user_id) {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_todos.find(user_id);
    if (it == m_todos.end()) return {};
    return it->second;
}
