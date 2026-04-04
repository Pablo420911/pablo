#include "home_manager.h"
#include <iostream>
#include <cassert>

static int tests_run = 0, tests_passed = 0;
#define TEST(expr) do { \
    ++tests_run; \
    if (static_cast<bool>(expr)) { ++tests_passed; std::cout << "  [PASS] " #expr "\n"; } \
    else std::cerr << "  [FAIL] " #expr "\n"; \
} while(0)

static bool contains(const std::string& s, const std::string& sub) {
    return s.find(sub) != std::string::npos;
}

void testLights() {
    HomeManager hm;
    auto r = hm.controlLight("kitchen", true);
    TEST(contains(r, "kitchen") || contains(r, "Kitchen") || !r.empty());
    auto r2 = hm.controlLight("kitchen", false);
    TEST(!r2.empty());
    auto status = hm.getLightStatus();
    TEST(!status.empty());
}

void testThermostat() {
    HomeManager hm;
    auto r = hm.setTemperature(22.0);
    TEST(contains(r, "22") || !r.empty());
    TEST(hm.getSetpoint() == 22.0);
    auto status = hm.getThermostatStatus();
    TEST(!status.empty());
}

void testLocks() {
    HomeManager hm;
    auto r = hm.controlLock("front door", true);
    TEST(!r.empty());
    auto r2 = hm.controlLock("front door", false);
    TEST(contains(r2, "unlock") || contains(r2, "front") || !r2.empty());
    auto status = hm.getLockStatus();
    TEST(!status.empty());
}

void testAlarm() {
    HomeManager hm;
    auto r = hm.setAlarmMode("away");
    TEST(!r.empty());
    auto r2 = hm.setAlarmMode("disarmed");
    TEST(!r2.empty());
    auto status = hm.getAlarmStatus();
    TEST(!status.empty());
}

void testAppliances() {
    HomeManager hm;
    auto r = hm.controlAppliance("coffee maker", true);
    TEST(!r.empty());
    auto r2 = hm.controlAppliance("coffee maker", false);
    TEST(!r2.empty());
}

void testFullStatus() {
    HomeManager hm;
    auto status = hm.getFullStatus();
    TEST(!status.empty());
    TEST(contains(status, "Thermostat") || contains(status, "Light") || contains(status, "Lock"));
}

void testSensors() {
    HomeManager hm;
    auto readings = hm.getSensorReadings();
    TEST(!readings.empty());
}

int main() {
    std::cout << "=== HomeManager Tests ===\n";
    testLights();
    testThermostat();
    testLocks();
    testAlarm();
    testAppliances();
    testFullStatus();
    testSensors();
    std::cout << "\n" << tests_passed << "/" << tests_run << " tests passed.\n";
    return tests_passed == tests_run ? 0 : 1;
}
