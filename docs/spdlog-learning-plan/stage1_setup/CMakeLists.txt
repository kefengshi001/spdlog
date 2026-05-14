cmake_minimum_required(VERSION 3.10)
project(spdlog_stage1_setup CXX)

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

# Create logs output directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/logs)

# Hello World demo
add_executable(hello_world src/hello_world.cpp)
target_link_libraries(hello_world PRIVATE spdlog::spdlog)

# Multi Sink demo
add_executable(multi_sink_demo src/multi_sink_demo.cpp)
target_link_libraries(multi_sink_demo PRIVATE spdlog::spdlog)

# Tests
enable_testing()

add_executable(test_install tests/test_install.cpp)
target_link_libraries(test_install PRIVATE spdlog::spdlog)
target_compile_definitions(test_install PRIVATE
    LOG_DIR="${CMAKE_BINARY_DIR}/logs"
)

add_test(NAME test_install COMMAND test_install)
