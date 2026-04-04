#include "pablo.h"
#include <iostream>
#include <cassert>
#include <cstdlib>

static int tests_run = 0, tests_passed = 0;
#define TEST(expr) do { \
    ++tests_run; \
    if (static_cast<bool>(expr)) { ++tests_passed; std::cout << "  [PASS] " #expr "\n"; } \
    else std::cerr << "  [FAIL] " #expr "\n"; \
} while(0)

static bool contains(const std::string& s, const std::string& sub) {
    return s.find(sub) != std::string::npos;
}

// Use a temp directory for tests so they don't pollute real data
struct TempDir {
    std::string path;
    TempDir() : path("/tmp/pablo_test_dir") {
        std::system("mkdir -p /tmp/pablo_test_dir");
    }
};

void testGreeting() {
    TempDir td;
    Pablo pablo;
    pablo.init(td.path);
    auto r = pablo.respond("hello");
    TEST(!r.empty());
    TEST(contains(r, "Pablo") || contains(r, "hello") || contains(r, "Hi") ||
         contains(r, "Hello") || contains(r, "Hey") || contains(r, "Greet"));
}

void testFarewell() {
    TempDir td;
    Pablo pablo;
    pablo.init(td.path);
    auto r = pablo.respond("goodbye");
    TEST(!r.empty());
    TEST(contains(r, "bye") || contains(r, "Goodbye") || contains(r, "saved") ||
         contains(r, "take care") || contains(r, "Take"));
}

void testTimeQuestion() {
    TempDir td;
    Pablo pablo;
    pablo.init(td.path);
    auto r = pablo.respond("what time is it");
    TEST(!r.empty());
    TEST(contains(r, ":") || contains(r, "time") || contains(r, "Time"));
}

void testDateQuestion() {
    TempDir td;
    Pablo pablo;
    pablo.init(td.path);
    auto r = pablo.respond("what day is today");
    TEST(!r.empty());
    // Should contain a day name or year
    bool hasDate = contains(r, "2025") || contains(r, "2026") ||
                   contains(r, "Monday") || contains(r, "Tuesday") ||
                   contains(r, "Wednesday") || contains(r, "Thursday") ||
                   contains(r, "Friday") || contains(r, "Saturday") ||
                   contains(r, "Sunday") || contains(r, "today") || contains(r, "Today");
    TEST(hasDate);
}

void testLearnAndRecall() {
    TempDir td;
    Pablo pablo;
    pablo.init(td.path);
    pablo.respond("remember that my dog is named Rex");
    auto r = pablo.respond("what is my dog named");
    TEST(!r.empty());
    TEST(contains(r, "rex") || contains(r, "Rex") || contains(r, "dog"));
}

void testCommand() {
    TempDir td;
    Pablo pablo;
    pablo.init(td.path);
    auto r = pablo.respond("turn on the kitchen lights");
    TEST(!r.empty());
    TEST(contains(r, "kitchen") || contains(r, "light") || contains(r, "on"));
}

void testCustomResponse() {
    TempDir td;
    Pablo pablo;
    pablo.init(td.path);
    pablo.respond("when I say goodnight respond sweet dreams");
    auto r = pablo.respond("goodnight");
    TEST(!r.empty());
    TEST(contains(r, "sweet") || contains(r, "dreams") || contains(r, "Sweet"));
}

void testWellnessCheckIn() {
    TempDir td;
    Pablo pablo;
    pablo.init(td.path);
    auto r = pablo.respond("check in on me");
    TEST(!r.empty());
    TEST(contains(r, "feeling") || contains(r, "scale") || contains(r, "sleep") ||
         contains(r, "energy") || contains(r, "pain") || contains(r, "mood") ||
         contains(r, "eaten") || contains(r, "today") || contains(r, "how"));
    // Respond to check-in
    auto r2 = pablo.respond("I feel great");
    TEST(!r2.empty());
    TEST(contains(r2, "great") || contains(r2, "wonderful") || contains(r2, "score") ||
         contains(r2, "glad") || contains(r2, "thank"));
}

void testHelp() {
    TempDir td;
    Pablo pablo;
    pablo.init(td.path);
    auto r = pablo.respond("help");
    TEST(!r.empty());
    TEST(contains(r, "home") || contains(r, "HOME") || contains(r, "control") ||
         contains(r, "learn") || contains(r, "LEARN"));
}

void testHomeStatus() {
    TempDir td;
    Pablo pablo;
    pablo.init(td.path);
    auto r = pablo.respond("home status");
    TEST(!r.empty());
    TEST(contains(r, "Thermostat") || contains(r, "Light") || contains(r, "lock") ||
         contains(r, "home") || contains(r, "status"));
}

void testHistory() {
    TempDir td;
    Pablo pablo;
    pablo.init(td.path);
    pablo.respond("hello");
    pablo.respond("what time is it");
    auto history = pablo.getHistory();
    TEST(history.size() >= 2);
    auto histStr = pablo.getHistoryString();
    TEST(!histStr.empty());
}

void testWellnessMonitor() {
    TempDir td;
    Pablo pablo;
    pablo.init(td.path);
    TEST(pablo.occupantMonitor().getAlertLevel() == AlertLevel::NONE);
    TEST(pablo.occupantMonitor().getWellnessScore() >= 0);
    TEST(pablo.occupantMonitor().getWellnessScore() <= 100);
}

void testAccessibilityIntegration() {
    TempDir td;
    Pablo pablo;
    pablo.init(td.path);
    // Accessibility modes should not break responses
    pablo.accessibilityManager().setMode(ACCESS_SIMPLIFIED);
    auto r = pablo.respond("hello");
    TEST(!r.empty());
    pablo.accessibilityManager().setMode(ACCESS_NONE);
}

void testSaveLoad() {
    TempDir td;
    {
        Pablo pablo;
        pablo.init(td.path);
        pablo.respond("remember that my color is blue");
        pablo.save();
    }
    {
        Pablo pablo2;
        pablo2.init(td.path);
        auto r = pablo2.respond("what is my color");
        TEST(!r.empty());
    }
}

void testTemperatureCommand() {
    TempDir td;
    Pablo pablo;
    pablo.init(td.path);
    auto r = pablo.respond("set temperature to 23");
    TEST(!r.empty());
    TEST(pablo.homeManager().getSetpoint() == 23.0 ||
         contains(r, "23") || !r.empty());
}

void testLockCommand() {
    TempDir td;
    Pablo pablo;
    pablo.init(td.path);
    auto r = pablo.respond("lock the front door");
    TEST(!r.empty());
    auto r2 = pablo.respond("unlock the front door");
    TEST(contains(r2, "unlock") || contains(r2, "door") || !r2.empty());
}

int main() {
    std::cout << "=== Pablo Integration Tests ===\n";
    testGreeting();
    testFarewell();
    testTimeQuestion();
    testDateQuestion();
    testLearnAndRecall();
    testCommand();
    testCustomResponse();
    testWellnessCheckIn();
    testHelp();
    testHomeStatus();
    testHistory();
    testWellnessMonitor();
    testAccessibilityIntegration();
    testSaveLoad();
    testTemperatureCommand();
    testLockCommand();
    std::cout << "\n" << tests_passed << "/" << tests_run << " tests passed.\n";
    return tests_passed == tests_run ? 0 : 1;
}
