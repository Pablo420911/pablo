#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

// ---------------------------------------------------------------------------
// HomeManager – simulates a smart-home automation layer.
//
// Devices managed:
//   Lights    – per-room on/off + brightness (0-100)
//   Thermostat – target temperature + current mode (heat/cool/off)
//   Locks     – front door, back door, garage
//   Alarm     – armed/disarmed/triggered
//   Appliances – e.g. coffee_maker, dishwasher, oven, tv
//   Sensors   – motion, smoke, flood (read-only simulation)
// ---------------------------------------------------------------------------

struct LightDevice {
    std::string room;
    bool on{false};
    int  brightness{100};   // 0-100 %
};

struct ThermostatDevice {
    double setpoint{20.0};  // °C
    double current{20.0};   // °C simulated
    std::string mode{"off"};// "heat", "cool", "off"
    std::string unit{"C"};  // "C" or "F"
};

struct LockDevice {
    std::string name;
    bool locked{true};
};

struct Alarm {
    bool armed{false};
    bool triggered{false};
    std::string mode{"home"};  // "home", "away", "night", "disarmed"
};

struct ApplianceDevice {
    std::string name;
    bool on{false};
    std::string status{"idle"};
};

struct SensorReading {
    std::string name;
    bool active{false};
    std::string description;
};

class HomeManager {
public:
    HomeManager();

    // ---- Lights ----
    std::string controlLight(const std::string& room, bool turnOn);
    std::string setLightBrightness(const std::string& room, int brightness);
    std::string getLightStatus(const std::string& room = "") const;

    // ---- Thermostat ----
    std::string setTemperature(double celsius);
    std::string setThermostatMode(const std::string& mode);
    std::string getThermostatStatus() const;
    std::string setTemperatureUnit(const std::string& unit);
    double      getSetpoint() const;  // current target temperature in °C

    // ---- Locks ----
    std::string controlLock(const std::string& door, bool lock);
    std::string getLockStatus(const std::string& door = "") const;

    // ---- Alarm ----
    std::string setAlarmMode(const std::string& mode);
    std::string getAlarmStatus() const;
    std::string triggerAlarm();
    std::string silenceAlarm();

    // ---- Appliances ----
    std::string controlAppliance(const std::string& name, bool turnOn);
    std::string getApplianceStatus(const std::string& name = "") const;

    // ---- Sensors ----
    std::string getSensorReadings() const;
    void simulateSensor(const std::string& name, bool active);

    // ---- Full home report ----
    std::string getFullStatus() const;

    // ---- Natural-language command dispatcher ----
    // Returns a human-readable response after applying the command.
    std::string processCommand(const std::string& subject,
                               const std::string& action,
                               const std::string& value = "");

    // Helper: normalise room/device name
    static std::string normalise(std::string s);

private:
    std::unordered_map<std::string, LightDevice>     lights_;
    ThermostatDevice                                  thermostat_;
    std::unordered_map<std::string, LockDevice>      locks_;
    Alarm                                             alarm_;
    std::unordered_map<std::string, ApplianceDevice> appliances_;
    std::unordered_map<std::string, SensorReading>   sensors_;

    void initDefaults();

    // Try to find closest matching room/device name
    std::string fuzzyMatch(const std::string& query,
                           const std::vector<std::string>& candidates) const;
};
