// Tests for AccessibilityManager
#include "accessibility.h"
#include <cassert>
#include <iostream>
#include <string>

static int tests_run    = 0;
static int tests_passed = 0;

#define TEST(expr) do { \
    ++tests_run; \
    bool _ok = static_cast<bool>(expr); \
    if (_ok) { ++tests_passed; std::cout << "  [PASS] " #expr "\n"; } \
    else     { std::cerr << "  [FAIL] " #expr "\n"; } \
} while(0)

static bool contains(const std::string& s, const std::string& sub) {
    return s.find(sub) != std::string::npos;
}

void testDefaultMode() {
    AccessibilityManager am;
    TEST(am.getMode() == ACCESS_NONE);
    TEST(!am.hasVoiceOut());
    TEST(!am.isBlindMode());
    TEST(!am.isDeafMode());
    TEST(!am.isSimplified());
    TEST(!am.isScreenReader());
    TEST(!am.isDeafBlind());
}

void testSetMode() {
    AccessibilityManager am;
    am.setMode(ACCESS_DEAF);
    TEST(am.isDeafMode());
    TEST(!am.isBlindMode());
    am.setMode(ACCESS_NONE);
    TEST(am.getMode() == ACCESS_NONE);
}

void testBlindImpliesVoice() {
    AccessibilityManager am;
    am.setMode(ACCESS_BLIND);
    TEST(am.isBlindMode());
    TEST(am.hasVoiceOut());
}

void testAddRemoveMode() {
    AccessibilityManager am;
    am.addMode(ACCESS_DEAF);
    am.addMode(ACCESS_SIMPLIFIED);
    TEST(am.isDeafMode());
    TEST(am.isSimplified());
    TEST(!am.isBlindMode());
    am.removeMode(ACCESS_SIMPLIFIED);
    TEST(!am.isSimplified());
    TEST(am.isDeafMode());
}

void testDeafBlind() {
    AccessibilityManager am;
    am.setMode(static_cast<unsigned>(ACCESS_BLIND) | static_cast<unsigned>(ACCESS_DEAF));
    TEST(am.isDeafBlind());
    TEST(am.isBlindMode());
    TEST(am.isDeafMode());
    TEST(am.hasVoiceOut());
}

void testStripBoxChars() {
    std::string text = "test";
    std::string stripped = AccessibilityManager::stripBoxChars(text);
    TEST(stripped == text);
    // String with bullet
    std::string withBullet = "\xe2\x80\xa2 item"; // UTF-8 bullet
    std::string stripped2 = AccessibilityManager::stripBoxChars(withBullet);
    TEST(stripped2.find("item") != std::string::npos);
    TEST(stripped2.find("*") != std::string::npos);
}

void testSimplify() {
    std::string s = "Hello.";
    TEST(AccessibilityManager::simplify(s) == s);

    // Use text that's definitely longer than the 120-char threshold
    std::string longText =
        "This is sentence one. This is sentence two. "
        "This is sentence three which should be truncated away from the output. "
        "And this is sentence four which should definitely not appear here at all.";
    // Verify it's actually long
    TEST(longText.size() > 120);
    std::string simplified = AccessibilityManager::simplify(longText);
    TEST(simplified.size() < longText.size());
    TEST(contains(simplified, "sentence one"));
}

void testDescribeMode() {
    AccessibilityManager am;
    TEST(contains(am.describeMode(), "Standard") || contains(am.describeMode(), "text"));
    am.setMode(ACCESS_BLIND);
    std::string d = am.describeMode();
    TEST(contains(d, "lind")); // "Blind" or "blind"
}

void testHelpText() {
    std::string h = AccessibilityManager::helpText();
    TEST(!h.empty());
    TEST(contains(h, "espeak") || contains(h, "ESPEAK"));
    TEST(contains(h, "deaf") || contains(h, "DEAF"));
}

void testParseCommand() {
    AccessibilityManager am;
    TEST(am.parseCommand("set mode blind"));
    TEST(am.isBlindMode());
    am.parseCommand("set mode normal");
    TEST(am.getMode() == ACCESS_NONE);

    TEST(am.parseCommand("set mode deaf"));
    TEST(am.isDeafMode());
    am.parseCommand("set mode normal");

    TEST(am.parseCommand("set mode simplified"));
    TEST(am.isSimplified());
    am.parseCommand("set mode normal");

    TEST(am.parseCommand("set mode screen reader"));
    TEST(am.isScreenReader());
    am.parseCommand("set mode normal");

    // Toggle voice
    TEST(am.parseCommand("set mode voice"));
    TEST(am.hasVoiceOut());
    TEST(am.parseCommand("set mode voice")); // toggle off
    TEST(!am.hasVoiceOut());

    // Unknown - should NOT match
    TEST(!am.parseCommand("turn on lights"));
}

void testDeafBlindCommand() {
    AccessibilityManager am;
    TEST(am.parseCommand("set mode deafblind"));
    TEST(am.isDeafBlind());
    TEST(am.isBlindMode());
    TEST(am.isDeafMode());
    TEST(am.isScreenReader());
}

int main() {
    std::cout << "=== Accessibility Tests ===\n";
    testDefaultMode();
    testSetMode();
    testBlindImpliesVoice();
    testAddRemoveMode();
    testDeafBlind();
    testStripBoxChars();
    testSimplify();
    testDescribeMode();
    testHelpText();
    testParseCommand();
    testDeafBlindCommand();
    std::cout << "\n" << tests_passed << "/" << tests_run << " tests passed.\n";
    return (tests_passed == tests_run) ? 0 : 1;
}
