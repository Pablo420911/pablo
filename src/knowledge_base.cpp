#include "knowledge_base.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <iostream>

// ---- static helpers --------------------------------------------------------

std::string KnowledgeBase::makeKey(const std::string& category,
                                   const std::string& key) {
    return toLower(category) + "::" + toLower(key);
}

std::string KnowledgeBase::trim(const std::string& s) {
    auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

std::string KnowledgeBase::toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

// ---- Construction ----------------------------------------------------------

KnowledgeBase::KnowledgeBase() {}

KnowledgeBase::KnowledgeBase(const std::string& filepath) {
    load(filepath);
}

// ---- Store / retrieve ------------------------------------------------------

void KnowledgeBase::store(const std::string& category,
                          const std::string& key,
                          const std::string& value) {
    Entry e;
    e.category  = trim(category);
    e.key       = trim(key);
    e.value     = trim(value);
    e.timestamp = std::time(nullptr);
    entries_[makeKey(category, key)] = std::move(e);
}

bool KnowledgeBase::query(const std::string& category,
                          const std::string& key,
                          std::string& outValue) const {
    auto it = entries_.find(makeKey(category, key));
    if (it == entries_.end()) return false;
    outValue = it->second.value;
    return true;
}

std::vector<KnowledgeBase::Entry>
KnowledgeBase::search(const std::string& keyword) const {
    std::string kw = toLower(trim(keyword));
    std::vector<Entry> results;
    for (auto& [k, e] : entries_) {
        if (toLower(e.key).find(kw) != std::string::npos ||
            toLower(e.value).find(kw) != std::string::npos ||
            toLower(e.category).find(kw) != std::string::npos) {
            results.push_back(e);
        }
    }
    return results;
}

std::vector<KnowledgeBase::Entry>
KnowledgeBase::getByCategory(const std::string& category) const {
    std::string cat = toLower(trim(category));
    std::vector<Entry> results;
    for (auto& [k, e] : entries_) {
        if (toLower(e.category) == cat) results.push_back(e);
    }
    return results;
}

bool KnowledgeBase::erase(const std::string& category,
                           const std::string& key) {
    return entries_.erase(makeKey(category, key)) > 0;
}

std::size_t KnowledgeBase::size() const { return entries_.size(); }
bool        KnowledgeBase::empty() const { return entries_.empty(); }

// ---- Persistence -----------------------------------------------------------

bool KnowledgeBase::load(const std::string& filepath) {
    filepath_ = filepath;
    std::ifstream f(filepath);
    if (!f.is_open()) return false;

    std::string line;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        // Format: category|key|value|timestamp
        std::istringstream ss(line);
        std::string cat, key, val, ts;
        if (!std::getline(ss, cat, '|') ||
            !std::getline(ss, key, '|') ||
            !std::getline(ss, val, '|')) {
            continue;
        }
        std::getline(ss, ts, '|');

        Entry e;
        e.category  = trim(cat);
        e.key       = trim(key);
        e.value     = trim(val);
        try { e.timestamp = static_cast<std::time_t>(std::stoll(ts)); }
        catch (...) { e.timestamp = 0; }

        entries_[makeKey(e.category, e.key)] = std::move(e);
    }
    return true;
}

bool KnowledgeBase::save(const std::string& filepath) const {
    std::ofstream f(filepath);
    if (!f.is_open()) return false;

    f << "# Pablo Knowledge Base – auto-generated, do not edit manually\n";
    for (auto& [k, e] : entries_) {
        // escape any pipe characters in value
        std::string safeVal = e.value;
        for (auto& c : safeVal) if (c == '|') c = ';';
        f << e.category << '|' << e.key << '|' << safeVal
          << '|' << static_cast<long long>(e.timestamp) << '\n';
    }
    return true;
}

bool KnowledgeBase::save() const {
    if (filepath_.empty()) return false;
    return save(filepath_);
}

// ---- Debug -----------------------------------------------------------------

std::string KnowledgeBase::dump() const {
    std::ostringstream out;
    for (auto& [k, e] : entries_) {
        out << "[" << e.category << "] " << e.key << " = " << e.value << "\n";
    }
    return out.str();
}
