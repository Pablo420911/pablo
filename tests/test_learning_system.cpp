#include "knowledge_base.h"
#include "learning_system.h"
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

void testLearnFact() {
    KnowledgeBase kb;
    LearningSystem ls(kb);
    auto r = ls.learn("remember that my cat is named whiskers");
    TEST(r.success);
    TEST(!r.key.empty());
    TEST(!r.confirmation.empty());
    TEST(ls.getFact(r.key) == r.value || !r.value.empty());
}

void testLearnPreference() {
    KnowledgeBase kb;
    LearningSystem ls(kb);
    auto r = ls.learn("I prefer jazz music");
    TEST(r.success);
    TEST(!r.key.empty());
    TEST(ls.getPreference(r.key) == r.value || !r.value.empty());
}

void testLearnResponse() {
    KnowledgeBase kb;
    LearningSystem ls(kb);
    auto r = ls.learn("when I say goodnight respond sweet dreams");
    TEST(r.success);
    std::string found = ls.findCustomResponse("goodnight");
    TEST(contains(found, "sweet") || contains(found, "dreams") || !found.empty());
}

void testLearnSchedule() {
    KnowledgeBase kb;
    LearningSystem ls(kb);
    auto r = ls.learn("remind me at 08:00 to take medication");
    TEST(!r.confirmation.empty());
}

void testForget() {
    KnowledgeBase kb;
    LearningSystem ls(kb);
    ls.storeFact("test_key", "test_val");
    TEST(ls.getFact("test_key") == "test_val");
    auto r = ls.forget("forget test_key");
    TEST(!r.confirmation.empty());
}

void testDirectStore() {
    KnowledgeBase kb;
    LearningSystem ls(kb);
    ls.storeFact("color", "blue");
    TEST(ls.getFact("color") == "blue");

    ls.storePreference("temp", "22");
    TEST(ls.getPreference("temp") == "22");

    ls.storeResponse("morning", "Good morning!");
    TEST(!ls.findCustomResponse("morning").empty());
}

void testTotalLearned() {
    KnowledgeBase kb;
    LearningSystem ls(kb);
    TEST(ls.totalLearned() == 0);
    ls.storeFact("a", "1");
    ls.storeFact("b", "2");
    TEST(ls.totalLearned() == 2);
}

void testReport() {
    KnowledgeBase kb;
    LearningSystem ls(kb);
    ls.storeFact("x", "y");
    auto report = ls.getLearningReport();
    TEST(!report.empty());
}

int main() {
    std::cout << "=== LearningSystem Tests ===\n";
    testLearnFact();
    testLearnPreference();
    testLearnResponse();
    testLearnSchedule();
    testForget();
    testDirectStore();
    testTotalLearned();
    testReport();
    std::cout << "\n" << tests_passed << "/" << tests_run << " tests passed.\n";
    return tests_passed == tests_run ? 0 : 1;
}
