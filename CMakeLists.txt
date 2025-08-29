cmake_minimum_required(VERSION 3.10)
project(TankBattleProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ======================
# Include directories
# ======================
include_directories(
    ${CMAKE_SOURCE_DIR}/UserCommon
    ${CMAKE_SOURCE_DIR}/common
)

# <-- Add this line here -->
add_definitions(-DSOURCE_DIR="${CMAKE_SOURCE_DIR}")


# ======================
# Source Files for Simulator (exclude GameManager and Algorithm)
# ======================
file(GLOB_RECURSE SIMULATOR_SOURCES
    Simulator/*.cpp
    UserCommon/*.cpp
    common/*.cpp
)

# ======================
# Simulator executable
# ======================
add_executable(simulator_322719139_211961057 ${SIMULATOR_SOURCES})

# Place the executable in build/Simulator/
set_target_properties(simulator_322719139_211961057 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Simulator
)


target_link_libraries(simulator_322719139_211961057
    pthread
    dl
    -rdynamic
)

# ======================
# GoogleTest Setup
# ======================
enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ======================
# Test executable (do NOT include Simulator sources!)
# ======================
file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)

file(GLOB_RECURSE COMMON_ONLY_SOURCES
    UserCommon/*.cpp
    common/*.cpp
)

add_executable(tests ${TEST_SOURCES} ${COMMON_ONLY_SOURCES})

target_link_libraries(tests
    gtest_main
    pthread
    dl
)
