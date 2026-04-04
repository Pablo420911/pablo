#include "knowledge_base.h"
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <string>

static int tests_run = 0, tests_passed = 0;
#define TEST(expr) do { \
    ++tests_run; \
    if (static_cast<bool>(expr)) { ++tests_passed; std::cout << "  [PASS] " #expr "\n"; } \
    else std::cerr << "  [FAIL] " #expr "\n"; \
} while(0)

void testStoreQuery() {
    KnowledgeBase kb;
    kb.store("fact", "sky color", "blue");
    std::string val;
    TEST(kb.query("fact", "sky color", val));
    TEST(val == "blue");
    TEST(!kb.query("fact", "nonexistent", val));
}

void testOverwrite() {
    KnowledgeBase kb;
    kb.store("fact", "key", "v1");
    kb.store("fact", "key", "v2");
    std::string val;
    kb.query("fact", "key", val);
    TEST(val == "v2");
}

void testErase() {
    KnowledgeBase kb;
    kb.store("fact", "temp_key", "temp_val");
    TEST(kb.erase("fact", "temp_key"));
    std::string val;
    TEST(!kb.query("fact", "temp_key", val));
}

void testSearch() {
    KnowledgeBase kb;
    kb.store("fact", "cat name", "whiskers");
    kb.store("fact", "dog name", "rex");
    auto results = kb.search("cat");
    TEST(!results.empty());
    TEST(results[0].key == "cat name");
}

void testGetByCategory() {
    KnowledgeBase kb;
    kb.store("preference", "music", "jazz");
    kb.store("preference", "temperature", "22");
    kb.store("fact", "sky", "blue");
    auto prefs = kb.getByCategory("preference");
    TEST(prefs.size() == 2);
    auto facts = kb.getByCategory("fact");
    TEST(facts.size() == 1);
}

void testPersistence() {
    const char* tmpfile = "/tmp/pablo_test_kb.kb";
    {
        KnowledgeBase kb;
        kb.store("fact", "persist_key", "persist_val");
        kb.save(tmpfile);
    }
    {
        KnowledgeBase kb2;
        kb2.load(tmpfile);
        std::string val;
        TEST(kb2.query("fact", "persist_key", val));
        TEST(val == "persist_val");
    }
    std::remove(tmpfile);
}

void testSize() {
    KnowledgeBase kb;
    TEST(kb.empty());
    kb.store("fact", "a", "1");
    kb.store("fact", "b", "2");
    TEST(kb.size() == 2);
    TEST(!kb.empty());
}

int main() {
    std::cout << "=== KnowledgeBase Tests ===\n";
    testStoreQuery();
    testOverwrite();
    testErase();
    testSearch();
    testGetByCategory();
    testPersistence();
    testSize();
    std::cout << "\n" << tests_passed << "/" << tests_run << " tests passed.\n";
    return tests_passed == tests_run ? 0 : 1;
}
