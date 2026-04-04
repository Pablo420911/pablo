#include "nlp_engine.h"
#include <iostream>
#include <cassert>

static int tests_run = 0, tests_passed = 0;
#define TEST(expr) do { \
    ++tests_run; \
    if (static_cast<bool>(expr)) { ++tests_passed; std::cout << "  [PASS] " #expr "\n"; } \
    else std::cerr << "  [FAIL] " #expr "\n"; \
} while(0)

void testGreetingIntent() {
    NLPEngine nlp;
    TEST(nlp.parse("hello pablo").intent == Intent::GREETING);
    TEST(nlp.parse("hi there").intent == Intent::GREETING);
    TEST(nlp.parse("hey").intent == Intent::GREETING);
}

void testFarewellIntent() {
    NLPEngine nlp;
    TEST(nlp.parse("goodbye").intent == Intent::FAREWELL);
    TEST(nlp.parse("bye").intent == Intent::FAREWELL);
}

void testCommandIntent() {
    NLPEngine nlp;
    TEST(nlp.parse("turn on the lights").intent == Intent::COMMAND);
    TEST(nlp.parse("turn off the fan").intent == Intent::COMMAND);
    TEST(nlp.parse("set temperature to 22").intent == Intent::COMMAND);
}

void testQuestionIntent() {
    NLPEngine nlp;
    auto timeIntent = nlp.parse("what time is it").intent;
    TEST(timeIntent == Intent::QUESTION || timeIntent == Intent::STATUS);
    TEST(nlp.parse("who are you").intent == Intent::QUESTION);
}

void testTeachFact() {
    NLPEngine nlp;
    TEST(nlp.parse("remember that my cat is named Whiskers").intent == Intent::TEACH_FACT);
}

void testTeachPreference() {
    NLPEngine nlp;
    TEST(nlp.parse("I prefer jazz music").intent == Intent::TEACH_PREFERENCE);
}

void testTeachSchedule() {
    NLPEngine nlp;
    TEST(nlp.parse("remind me at 08:00 daily").intent == Intent::TEACH_SCHEDULE);
}

void testTeachResponse() {
    NLPEngine nlp;
    TEST(nlp.parse("when I say goodnight respond sweet dreams").intent == Intent::TEACH_RESPONSE);
}

void testForget() {
    NLPEngine nlp;
    TEST(nlp.parse("forget that my cat is named whiskers").intent == Intent::FORGET);
}

void testHelpIntent() {
    NLPEngine nlp;
    TEST(nlp.parse("help").intent == Intent::HELP);
    TEST(nlp.parse("what can you do").intent == Intent::HELP);
}

void testListLearned() {
    NLPEngine nlp;
    TEST(nlp.parse("show me what you know").intent == Intent::LIST_LEARNED);
}

void testAffirmation() {
    NLPEngine nlp;
    TEST(nlp.parse("yes").intent == Intent::AFFIRMATION);
    TEST(nlp.parse("ok").intent == Intent::AFFIRMATION);
}

void testNegation() {
    NLPEngine nlp;
    TEST(nlp.parse("no").intent == Intent::NEGATION);
}

void testTokenise() {
    auto tokens = NLPEngine::tokenise("Hello, World! How are you?");
    TEST(!tokens.empty());
    TEST(tokens[0] == "hello");
}

int main() {
    std::cout << "=== NLPEngine Tests ===\n";
    testGreetingIntent();
    testFarewellIntent();
    testCommandIntent();
    testQuestionIntent();
    testTeachFact();
    testTeachPreference();
    testTeachSchedule();
    testTeachResponse();
    testForget();
    testHelpIntent();
    testListLearned();
    testAffirmation();
    testNegation();
    testTokenise();
    std::cout << "\n" << tests_passed << "/" << tests_run << " tests passed.\n";
    return tests_passed == tests_run ? 0 : 1;
}
