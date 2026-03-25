cmake_minimum_required(VERSION 3.10)
project(RefinedExplorer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include headers
include_directories(include)

# ─── 1. CORE LIBRARY & TUI ───
# Separate business logic from main() and terminal UI
file(GLOB_RECURSE CORE_SOURCES
    src/core/*.cpp
    src/index/*.cpp
    src/utils/*.cpp
)
file(GLOB_RECURSE TUI_SOURCES
    src/tui/*.cpp
)

add_library(core_lib STATIC ${CORE_SOURCES})
target_link_libraries(core_lib
    pthread
    lmdb
)
if(APPLE)
    target_link_libraries(core_lib "-framework CoreServices")
endif()
target_compile_definitions(core_lib PRIVATE
    CONFIG_PATH="${CMAKE_SOURCE_DIR}/config.json"
)

# ─── 2. MAIN EXECUTABLE ───
add_executable(refined_explorer main.cpp ${TUI_SOURCES})
target_link_libraries(refined_explorer core_lib)

# ─── 3. GOOGLETEST & TESTS_RUNNER ───
include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0)
# For Windows: prevent gtest from overriding the MSVC runtime settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ─── 4. LMDB (LOCAL BUILD) ───
FetchContent_Declare(
    lmdb_proc
    URL https://github.com/LMDB/lmdb/archive/refs/tags/LMDB_0.9.32.tar.gz
)
FetchContent_MakeAvailable(lmdb_proc)

set(LMDB_SOURCE_DIR "${lmdb_proc_SOURCE_DIR}/libraries/liblmdb")
add_library(lmdb STATIC 
    ${LMDB_SOURCE_DIR}/mdb.c 
    ${LMDB_SOURCE_DIR}/midl.c
)
target_include_directories(lmdb PUBLIC ${LMDB_SOURCE_DIR})
# LMDB requires pthread on UNIX systems
if(UNIX)
    target_link_libraries(lmdb PRIVATE pthread)
endif()

# ─── 5. NLOHMANN JSON ───
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)
target_link_libraries(core_lib nlohmann_json::nlohmann_json)

add_executable(tests_runner
    tests/test_main.cpp
    tests/stubs.cpp
    tests/test_utils.cpp
    tests/test_file_utils.cpp
    tests/test_navigation.cpp
    tests/test_dir_cache.cpp
    tests/test_commands.cpp
    tests/test_search.cpp
    tests/test_index.cpp
    tests/test_config.cpp
    tests/test_selection.cpp
    tests/stubs.cpp
)
target_link_libraries(tests_runner core_lib GTest::gtest_main)
target_compile_definitions(tests_runner PRIVATE
    DUMMY_DIR="${CMAKE_SOURCE_DIR}/tests/dummy")

# Automatically run tests after build and output to JSON, then cleanup dummy data
add_custom_command(TARGET tests_runner POST_BUILD
    COMMAND tests_runner --gtest_output=json:test_results.json
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_SOURCE_DIR}/tests/dummy"
    COMMENT "Running automated tests with JSON output and cleaning up dummy folder..."
)