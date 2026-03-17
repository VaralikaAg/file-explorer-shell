cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_COMPILER /opt/homebrew/bin/gcc-15)
set(CMAKE_CXX_COMPILER /opt/homebrew/bin/g++-15)

project(RefinedExplorer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include headers
include_directories(include)

# Collect all source files
file(GLOB_RECURSE SOURCES
    src/*.cpp
    main.cpp
)

# Create executable
add_executable(refined_explorer ${SOURCES})

# Link required system libraries (Linux)
target_link_libraries(refined_explorer
    pthread
)

target_compile_definitions(refined_explorer PRIVATE
    CONFIG_PATH="${CMAKE_SOURCE_DIR}/config.yml"
)