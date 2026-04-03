#include "home_manager.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <cmath>

// ---- Normalise helper -------------------------------------------------------

std::string HomeManager::normalise(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    // replace underscores/hyphens with spaces
    for (auto& c : s) if (c == '_' || c == '-') c = ' ';
    // trim
    auto b = s.find_first_not_of(' ');
    auto e = s.find_last_not_of(' ');
    return (b == std::string::npos) ? "" : s.substr(b, e - b + 1);
}

// ---- Fuzzy match ------------------------------------------------------------

std::string HomeManager::fuzzyMatch(const std::string& query,
                                     const std::vector<std::string>& candidates) const {
    std::string q = normalise(query);
    for (auto& c : candidates) {
        std::string nc = normalise(c);
        if (nc == q || nc.find(q) != std::string::npos ||
            q.find(nc) != std::string::npos) {
            return c;
        }
    }
    return "";
}

// ---- Initialisation ---------------------------------------------------------

HomeManager::HomeManager() { initDefaults(); }

void HomeManager::initDefaults() {
    // Lights
    for (auto& room : {"living room", "bedroom", "kitchen", "bathroom",
                        "hallway", "office", "garage"}) {
        LightDevice l;
        l.room = room;
        l.on   = false;
        l.brightness = 100;
        lights_[room] = l;
    }

    // Thermostat
    thermostat_.setpoint = 21.0;
    thermostat_.current  = 20.5;
    thermostat_.mode     = "off";
    thermostat_.unit     = "C";

    // Locks
    for (auto& door : {"front door", "back door", "garage"}) {
        LockDevice lk;
        lk.name   = door;
        lk.locked = true;
        locks_[door] = lk;
    }

    // Alarm
    alarm_.armed     = false;
    alarm_.triggered = false;
    alarm_.mode      = "disarmed";

    // Appliances
    for (auto& name : {"coffee maker", "dishwasher", "oven", "tv",
                        "microwave", "washer", "dryer"}) {
        ApplianceDevice a;
        a.name   = name;
        a.on     = false;
        a.status = "off";
        appliances_[name] = a;
    }

    // Sensors
    for (auto& [name, desc] : std::vector<std::pair<std::string,std::string>>{
            {"motion",  "Motion sensor – hallway"},
            {"smoke",   "Smoke detector – kitchen"},
            {"flood",   "Flood sensor – basement"},
            {"co2",     "CO2 sensor – main floor"},
    }) {
        SensorReading sr;
        sr.name        = name;
        sr.active      = false;
        sr.description = desc;
        sensors_[name] = sr;
    }
}

// ---- Lights -----------------------------------------------------------------

std::string HomeManager::controlLight(const std::string& room, bool turnOn) {
    std::string r = normalise(room);
    std::vector<std::string> rooms;
    for (auto& [k, v] : lights_) rooms.push_back(k);
    std::string match = r.empty() ? "" : fuzzyMatch(r, rooms);

    if (!match.empty()) {
        lights_[match].on = turnOn;
        return "I've turned " + std::string(turnOn ? "on" : "off") +
               " the " + match + " light.";
    }
    if (r.empty() || r == "all" || r == "everything") {
        for (auto& [k, v] : lights_) v.on = turnOn;
        return std::string("I've turned ") + (turnOn ? "on" : "off") +
               " all the lights.";
    }
    return "I couldn't find a light for room \"" + room +
           "\". Known rooms are: living room, bedroom, kitchen, bathroom, "
           "hallway, office, garage.";
}

std::string HomeManager::setLightBrightness(const std::string& room, int brightness) {
    brightness = std::max(0, std::min(100, brightness));
    std::string r = normalise(room);
    std::vector<std::string> rooms;
    for (auto& [k, v] : lights_) rooms.push_back(k);
    std::string match = fuzzyMatch(r, rooms);

    if (!match.empty()) {
        lights_[match].brightness = brightness;
        lights_[match].on = brightness > 0;
        return "I've set the " + match + " light to " +
               std::to_string(brightness) + "% brightness.";
    }
    return "I couldn't find a light for room \"" + room + "\".";
}

std::string HomeManager::getLightStatus(const std::string& room) const {
    std::ostringstream oss;
    if (room.empty()) {
        oss << "Lights status:\n";
        for (auto& [k, v] : lights_) {
            oss << "  " << k << ": "
                << (v.on ? "ON (" + std::to_string(v.brightness) + "%)" : "OFF")
                << "\n";
        }
    } else {
        std::string r = normalise(room);
        std::vector<std::string> rooms;
        for (auto& [k, v] : lights_) rooms.push_back(k);
        std::string match = const_cast<HomeManager*>(this)->fuzzyMatch(r, rooms);
        if (!match.empty()) {
            auto& v = lights_.at(match);
            oss << match << " light: "
                << (v.on ? "ON (" + std::to_string(v.brightness) + "%)" : "OFF");
        } else {
            oss << "No light found for room \"" << room << "\".";
        }
    }
    return oss.str();
}

// ---- Thermostat -------------------------------------------------------------

std::string HomeManager::setTemperature(double celsius) {
    thermostat_.setpoint = celsius;
    if (celsius > thermostat_.current) thermostat_.mode = "heat";
    else if (celsius < thermostat_.current) thermostat_.mode = "cool";
    double display = celsius;
    std::string unit = thermostat_.unit;
    if (unit == "F") display = celsius * 9.0 / 5.0 + 32.0;
    std::ostringstream oss;
    oss.precision(1);
    oss << std::fixed;
    oss << "Thermostat set to " << display << "°" << unit
        << " (mode: " << thermostat_.mode << ").";
    return oss.str();
}

std::string HomeManager::setThermostatMode(const std::string& mode) {
    std::string m = normalise(mode);
    if (m == "heat" || m == "cool" || m == "off" || m == "auto") {
        thermostat_.mode = m;
        return "Thermostat mode set to " + m + ".";
    }
    return "Unknown thermostat mode \"" + mode +
           "\". Options: heat, cool, off, auto.";
}

std::string HomeManager::getThermostatStatus() const {
    std::ostringstream oss;
    oss.precision(1);
    oss << std::fixed;
    double sp = thermostat_.setpoint;
    double cur = thermostat_.current;
    if (thermostat_.unit == "F") {
        sp  = sp  * 9.0 / 5.0 + 32.0;
        cur = cur * 9.0 / 5.0 + 32.0;
    }
    oss << "Thermostat: setpoint " << sp << "°" << thermostat_.unit
        << ", current " << cur << "°" << thermostat_.unit
        << ", mode: " << thermostat_.mode << ".";
    return oss.str();
}

std::string HomeManager::setTemperatureUnit(const std::string& unit) {
    std::string u = normalise(unit);
    if (u == "f" || u == "fahrenheit") { thermostat_.unit = "F"; return "Temperature unit set to Fahrenheit."; }
    if (u == "c" || u == "celsius")    { thermostat_.unit = "C"; return "Temperature unit set to Celsius."; }
    return "Unknown unit \"" + unit + "\". Use C or F.";
}

double HomeManager::getSetpoint() const {
    return thermostat_.setpoint;
}

// ---- Locks ------------------------------------------------------------------

std::string HomeManager::controlLock(const std::string& door, bool lock) {
    std::string d = normalise(door);
    std::vector<std::string> doors;
    for (auto& [k, v] : locks_) doors.push_back(k);
    std::string match = d.empty() ? "" : fuzzyMatch(d, doors);

    if (!match.empty()) {
        locks_[match].locked = lock;
        return (lock ? "I've locked " : "I've unlocked ") + match + ".";
    }
    if (d.empty() || d == "all" || d == "everything") {
        for (auto& [k, v] : locks_) v.locked = lock;
        return std::string(lock ? "All doors locked." : "All doors unlocked.");
    }
    return "I couldn't find a lock named \"" + door + "\".";
}

std::string HomeManager::getLockStatus(const std::string& door) const {
    std::ostringstream oss;
    if (door.empty()) {
        oss << "Lock status:\n";
        for (auto& [k, v] : locks_) {
            oss << "  " << k << ": " << (v.locked ? "LOCKED" : "UNLOCKED") << "\n";
        }
    } else {
        std::string d = normalise(door);
        std::vector<std::string> doors;
        for (auto& [k, v] : locks_) doors.push_back(k);
        std::string match = const_cast<HomeManager*>(this)->fuzzyMatch(d, doors);
        if (!match.empty()) {
            oss << match << " is " << (locks_.at(match).locked ? "LOCKED" : "UNLOCKED") << ".";
        } else {
            oss << "No lock found for \"" << door << "\".";
        }
    }
    return oss.str();
}

// ---- Alarm ------------------------------------------------------------------

std::string HomeManager::setAlarmMode(const std::string& mode) {
    std::string m = normalise(mode);
    if (m == "arm" || m == "armed" || m == "away") {
        alarm_.armed = true;
        alarm_.mode  = "away";
        return "Security alarm armed in away mode.";
    }
    if (m == "home") {
        alarm_.armed = true;
        alarm_.mode  = "home";
        return "Security alarm armed in home mode.";
    }
    if (m == "night") {
        alarm_.armed = true;
        alarm_.mode  = "night";
        return "Security alarm armed in night mode.";
    }
    if (m == "disarm" || m == "disarmed" || m == "off") {
        alarm_.armed     = false;
        alarm_.triggered = false;
        alarm_.mode      = "disarmed";
        return "Security alarm disarmed.";
    }
    return "Unknown alarm mode \"" + mode + "\". Options: home, away, night, disarm.";
}

std::string HomeManager::getAlarmStatus() const {
    std::string status = alarm_.armed ?
        ("ARMED (" + alarm_.mode + ")") : "DISARMED";
    if (alarm_.triggered) status += " – TRIGGERED!";
    return "Alarm: " + status + ".";
}

std::string HomeManager::triggerAlarm() {
    alarm_.triggered = true;
    return "ALARM TRIGGERED! All zones active.";
}

std::string HomeManager::silenceAlarm() {
    alarm_.triggered = false;
    return "Alarm silenced.";
}

// ---- Appliances -------------------------------------------------------------

std::string HomeManager::controlAppliance(const std::string& name, bool turnOn) {
    std::string n = normalise(name);
    std::vector<std::string> apps;
    for (auto& [k, v] : appliances_) apps.push_back(k);
    std::string match = fuzzyMatch(n, apps);

    if (!match.empty()) {
        appliances_[match].on     = turnOn;
        appliances_[match].status = turnOn ? "on" : "off";
        return (turnOn ? "I've turned on the " : "I've turned off the ") +
               match + ".";
    }
    return "I don't know the appliance \"" + name +
           "\". Known appliances: coffee maker, dishwasher, oven, tv, "
           "microwave, washer, dryer.";
}

std::string HomeManager::getApplianceStatus(const std::string& name) const {
    std::ostringstream oss;
    if (name.empty()) {
        oss << "Appliance status:\n";
        for (auto& [k, v] : appliances_) {
            oss << "  " << k << ": " << (v.on ? "ON" : "OFF") << "\n";
        }
    } else {
        std::string n = normalise(name);
        std::vector<std::string> apps;
        for (auto& [k, v] : appliances_) apps.push_back(k);
        std::string match = const_cast<HomeManager*>(this)->fuzzyMatch(n, apps);
        if (!match.empty()) {
            oss << match << " is " << (appliances_.at(match).on ? "ON" : "OFF") << ".";
        } else {
            oss << "No appliance found named \"" << name << "\".";
        }
    }
    return oss.str();
}

// ---- Sensors ----------------------------------------------------------------

std::string HomeManager::getSensorReadings() const {
    std::ostringstream oss;
    oss << "Sensor readings:\n";
    for (auto& [k, v] : sensors_) {
        oss << "  " << v.description << ": "
            << (v.active ? "ACTIVE" : "clear") << "\n";
    }
    return oss.str();
}

void HomeManager::simulateSensor(const std::string& name, bool active) {
    std::string n = normalise(name);
    if (sensors_.count(n)) sensors_[n].active = active;
}

// ---- Full status report -----------------------------------------------------

std::string HomeManager::getFullStatus() const {
    std::ostringstream oss;
    oss << "=== Home Status ===\n";
    oss << getThermostatStatus() << "\n";
    oss << getLightStatus() << "\n";
    oss << getLockStatus() << "\n";
    oss << getAlarmStatus() << "\n";
    oss << getApplianceStatus() << "\n";
    oss << getSensorReadings();
    return oss.str();
}

// ---- Natural-language command dispatcher ------------------------------------

std::string HomeManager::processCommand(const std::string& subject,
                                         const std::string& action,
                                         const std::string& value) {
    std::string s = normalise(subject);
    std::string a = normalise(action);
    std::string v = normalise(value);

    // Lights
    if (s.find("light") != std::string::npos ||
        s.find("lamp") != std::string::npos) {
        if (a == "on" || a == "turn on") return controlLight(s, true);
        if (a == "off" || a == "turn off") return controlLight(s, false);
        if (a == "status") return getLightStatus(s);
        if (!v.empty()) {
            try { return setLightBrightness(s, std::stoi(v)); }
            catch (...) {}
        }
        return getLightStatus(s);
    }
    // Thermostat
    if (s.find("thermostat") != std::string::npos ||
        s.find("temperature") != std::string::npos ||
        s.find("heat") != std::string::npos ||
        s.find("cool") != std::string::npos) {
        if (!v.empty()) {
            try { return setTemperature(std::stod(v)); }
            catch (...) {}
        }
        if (!a.empty()) return setThermostatMode(a);
        return getThermostatStatus();
    }
    // Locks
    if (s.find("lock") != std::string::npos ||
        s.find("door") != std::string::npos) {
        if (a == "lock") return controlLock(s, true);
        if (a == "unlock") return controlLock(s, false);
        return getLockStatus(s);
    }
    // Alarm
    if (s.find("alarm") != std::string::npos ||
        s.find("security") != std::string::npos) {
        if (!a.empty()) return setAlarmMode(a);
        return getAlarmStatus();
    }
    // Appliances
    if (a == "on" || a == "turn on") return controlAppliance(s, true);
    if (a == "off" || a == "turn off") return controlAppliance(s, false);

    return getFullStatus();
}
