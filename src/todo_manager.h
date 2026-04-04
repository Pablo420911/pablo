#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

struct TodoItem {
    int         id;
    std::string task;
    bool        done;
    std::string created_at;   // ISO-8601 UTC string
};

class TodoManager {
public:
    // data_path: directory where todos.json is stored
    explicit TodoManager(const std::string& data_path);

    // Returns the new item's id
    int  addTodo(const std::string& user_id, const std::string& task);

    // Returns false if id not found
    bool markDone(const std::string& user_id, int id);

    // Removes all completed todos for a user
    void clearDone(const std::string& user_id);

    // Returns false if id not found
    bool removeTodo(const std::string& user_id, int id);

    std::vector<TodoItem> getTodos(const std::string& user_id);

private:
    void load();
    void save() const;

    std::string m_path;  // full path to todos.json
    mutable std::mutex m_mutex;

    // user_id -> list of items
    std::unordered_map<std::string, std::vector<TodoItem>> m_todos;
};
