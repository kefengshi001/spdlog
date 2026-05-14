cmake_minimum_required(VERSION 3.10)
project(spdlog_stage4_porting CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch spdlog
include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(spdlog)

# Find threads for multi-threading
find_package(Threads REQUIRED)

# Create logs output directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/logs)

# RoboLog library
add_library(robo_log STATIC
    src/robo_log.cpp
)
target_link_libraries(robo_log PUBLIC spdlog::spdlog Threads::Threads)
target_include_directories(robo_log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Demo executable
add_executable(main_demo src/main_demo.cpp)
target_link_libraries(main_demo PRIVATE robo_log)

# Tests
enable_testing()

add_executable(test_robo_log tests/test_robo_log.cpp)
target_link_libraries(test_robo_log PRIVATE robo_log)
target_compile_definitions(test_robo_log PRIVATE
    LOG_DIR="${CMAKE_BINARY_DIR}/logs"
)
add_test(NAME test_robo_log COMMAND test_robo_log)

add_executable(test_performance tests/test_performance.cpp)
target_link_libraries(test_performance PRIVATE robo_log)
target_compile_definitions(test_performance PRIVATE
    LOG_DIR="${CMAKE_BINARY_DIR}/logs"
)
add_test(NAME test_performance COMMAND test_performance)
