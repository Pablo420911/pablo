cmake_minimum_required(VERSION 3.14)
project(Pablo VERSION 1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Threads (required for autonomous background tick loop)
find_package(Threads REQUIRED)

# ---- Library (all modules except main) --------------------------------------
set(PABLO_SOURCES
    src/knowledge_base.cpp
    src/nlp_engine.cpp
    src/home_manager.cpp
    src/occupant_monitor.cpp
    src/learning_system.cpp
    src/accessibility.cpp
    src/pablo.cpp
)

add_library(pablo_lib STATIC ${PABLO_SOURCES})
target_include_directories(pablo_lib PUBLIC include)
target_link_libraries(pablo_lib PUBLIC Threads::Threads)

# ---- Executable -------------------------------------------------------------
add_executable(pablo src/main.cpp)
target_link_libraries(pablo PRIVATE pablo_lib)

# Copy data directory next to the binary
file(COPY data DESTINATION ${CMAKE_BINARY_DIR})

# ---- Tests ------------------------------------------------------------------
enable_testing()

# Test: knowledge base
add_executable(test_knowledge_base tests/test_knowledge_base.cpp)
target_link_libraries(test_knowledge_base PRIVATE pablo_lib)
add_test(NAME KnowledgeBase COMMAND test_knowledge_base)

# Test: NLP engine
add_executable(test_nlp_engine tests/test_nlp_engine.cpp)
target_link_libraries(test_nlp_engine PRIVATE pablo_lib)
add_test(NAME NLPEngine COMMAND test_nlp_engine)

# Test: home manager
add_executable(test_home_manager tests/test_home_manager.cpp)
target_link_libraries(test_home_manager PRIVATE pablo_lib)
add_test(NAME HomeManager COMMAND test_home_manager)

# Test: learning system
add_executable(test_learning tests/test_learning_system.cpp)
target_link_libraries(test_learning PRIVATE pablo_lib)
add_test(NAME LearningSystem COMMAND test_learning)

# Test: pablo integration
add_executable(test_pablo tests/test_pablo.cpp)
target_link_libraries(test_pablo PRIVATE pablo_lib)
add_test(NAME PabloIntegration COMMAND test_pablo)

# Test: accessibility
add_executable(test_accessibility tests/test_accessibility.cpp)
target_link_libraries(test_accessibility PRIVATE pablo_lib)
add_test(NAME Accessibility COMMAND test_accessibility)
