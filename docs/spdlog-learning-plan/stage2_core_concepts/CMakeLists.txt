cmake_minimum_required(VERSION 3.10)
project(spdlog_stage2_core_concepts CXX)

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

# Demo executables
add_executable(logger_demo src/logger_demo.cpp)
target_link_libraries(logger_demo PRIVATE spdlog::spdlog)

add_executable(sink_demo src/sink_demo.cpp)
target_link_libraries(sink_demo PRIVATE spdlog::spdlog)

add_executable(formatter_demo src/formatter_demo.cpp)
target_link_libraries(formatter_demo PRIVATE spdlog::spdlog)

add_executable(async_demo src/async_demo.cpp)
target_link_libraries(async_demo PRIVATE spdlog::spdlog)

# Tests
enable_testing()

add_executable(test_core_concepts tests/test_core_concepts.cpp)
target_link_libraries(test_core_concepts PRIVATE spdlog::spdlog)
target_compile_definitions(test_core_concepts PRIVATE
    LOG_DIR="${CMAKE_BINARY_DIR}/logs"
)

add_test(NAME test_core_concepts COMMAND test_core_concepts)
