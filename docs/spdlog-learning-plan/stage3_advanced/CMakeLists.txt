cmake_minimum_required(VERSION 3.10)
project(spdlog_stage3_advanced CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(spdlog REQUIRED)

# Create logs output directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/logs)

# Demo executables
add_executable(custom_sink src/custom_sink.cpp)
target_link_libraries(custom_sink PRIVATE spdlog::spdlog)

add_executable(custom_formatter src/custom_formatter.cpp)
target_link_libraries(custom_formatter PRIVATE spdlog::spdlog)

add_executable(async_tuning src/async_tuning.cpp)
target_link_libraries(async_tuning PRIVATE spdlog::spdlog)

add_executable(log_rotation src/log_rotation.cpp)
target_link_libraries(log_rotation PRIVATE spdlog::spdlog)

add_executable(thread_safety src/thread_safety.cpp)
target_link_libraries(thread_safety PRIVATE spdlog::spdlog Threads::Threads)
find_package(Threads REQUIRED)

# Tests
enable_testing()

add_executable(test_advanced tests/test_advanced.cpp)
target_link_libraries(test_advanced PRIVATE spdlog::spdlog Threads::Threads)
target_compile_definitions(test_advanced PRIVATE
    LOG_DIR="${CMAKE_BINARY_DIR}/logs"
)

add_test(NAME test_advanced COMMAND test_advanced)
