cmake_minimum_required(VERSION 3.15)
project(RoninCraftBot VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# --- D++ (DPP) Discord library ---
FetchContent_Declare(
    dpp
    GIT_REPOSITORY https://github.com/brainboxdotcc/DPP.git
    GIT_TAG        v10.0.35
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(dpp)

# --- nlohmann/json (header-only, single-file) ---
FetchContent_Declare(
    json
    URL      https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
)
FetchContent_MakeAvailable(json)

add_executable(RoninCraftBot
    src/main.cpp
    src/todo_manager.cpp
    src/reminder_manager.cpp
)

target_include_directories(RoninCraftBot PRIVATE src)

target_link_libraries(RoninCraftBot
    PRIVATE
        dpp
        nlohmann_json::nlohmann_json
)

# Ensure data directory exists next to the binary
add_custom_command(TARGET RoninCraftBot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:RoninCraftBot>/data
    COMMENT "Creating data directory"
)
