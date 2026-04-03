#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>

// ---------------------------------------------------------------------------
// KnowledgeBase – persistent key-value store organised by category.
//
// Categories used throughout Pablo:
//   "fact"        – general world knowledge learned from the occupant
//   "preference"  – occupant preferences (temperature, lighting, music …)
//   "response"    – custom question → answer pairs taught by the occupant
//   "schedule"    – recurring events (e.g. "morning_routine=07:00")
//   "user"        – personal info about the occupant
// ---------------------------------------------------------------------------

class KnowledgeBase {
public:
    struct Entry {
        std::string category;
        std::string key;
        std::string value;
        std::time_t timestamp{0};
    };

    KnowledgeBase();
    explicit KnowledgeBase(const std::string& filepath);

    // Store / retrieve entries
    void store(const std::string& category,
               const std::string& key,
               const std::string& value);

    bool query(const std::string& category,
               const std::string& key,
               std::string& outValue) const;

    // Fuzzy keyword search – returns all matching entries
    std::vector<Entry> search(const std::string& keyword) const;

    // Returns all entries in a given category
    std::vector<Entry> getByCategory(const std::string& category) const;

    // Erase an entry
    bool erase(const std::string& category, const std::string& key);

    // Persistence
    bool load(const std::string& filepath);
    bool save(const std::string& filepath) const;
    bool save() const;                // save to the path used at load/construction

    std::size_t size() const;
    bool empty() const;

    // Dump all entries for debugging
    std::string dump() const;

private:
    // compound key: "category::key"
    std::unordered_map<std::string, Entry> entries_;
    std::string filepath_;

    static std::string makeKey(const std::string& category,
                               const std::string& key);

    static std::string trim(const std::string& s);
    static std::string toLower(std::string s);
};
