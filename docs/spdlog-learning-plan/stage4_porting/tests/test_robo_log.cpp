// RoboLog 单元测试

#include "robo_log.h"
#include "robo_log_config.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

#ifndef LOG_DIR
#define LOG_DIR "."
#endif

void test_init_shutdown() {
    RoboLog::init(debug_config());
    assert(true);  // No crash
    RoboLog::shutdown();
    std::cout << "  [PASS] init/shutdown" << std::endl;
}

void test_log_levels() {
    RoboLog::init(debug_config());

    RoboLog::trace("trace message");
    RoboLog::debug("debug message");
    RoboLog::info("info message");
    RoboLog::warn("warn message");
    RoboLog::error("error message");
    RoboLog::critical("critical message");

    RoboLog::shutdown();
    std::cout << "  [PASS] log levels" << std::endl;
}

void test_formatted_logging() {
    RoboLog::init(debug_config());

    RoboLog::info("Joint {}: angle={:.2f}°", "J1", 45.678);
    RoboLog::warn("Speed near limit: {:.2f} rad/s", 2.95);
    RoboLog::error("Timeout: {}ms", 150);

    RoboLog::shutdown();
    std::cout << "  [PASS] formatted logging" << std::endl;
}

void test_file_output() {
    std::string log_path = std::string(LOG_DIR) + "/test_robo.log";
    std::string error_path = std::string(LOG_DIR) + "/test_robo_error.log";

    RoboLogConfig config;
    config.console_enabled = false;
    config.file_enabled = true;
    config.file_path = log_path;
    config.error_file_enabled = true;
    config.error_file_path = error_path;
    config.async_enabled = false;

    RoboLog::init(config);
    RoboLog::info("Test message");
    RoboLog::error("Error message");
    RoboLog::flush();
    RoboLog::shutdown();

    // Verify files exist
    std::ifstream log_file(log_path);
    assert(log_file.is_open());
    log_file.close();

    std::ifstream error_file(error_path);
    assert(error_file.is_open());
    error_file.close();

    std::cout << "  [PASS] file output" << std::endl;
}

void test_level_filtering() {
    std::string log_path = std::string(LOG_DIR) + "/test_level_filter.log";

    RoboLogConfig config;
    config.console_enabled = false;
    config.file_enabled = true;
    config.file_path = log_path;
    config.error_file_enabled = false;
    config.level = spdlog::level::warn;
    config.async_enabled = false;

    RoboLog::init(config);
    RoboLog::debug("should not appear");
    RoboLog::info("should not appear");
    RoboLog::warn("should appear");
    RoboLog::error("should appear");
    RoboLog::flush();
    RoboLog::shutdown();

    std::ifstream file(log_path);
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    assert(content.find("should not appear") == std::string::npos);
    assert(content.find("should appear") != std::string::npos);

    std::cout << "  [PASS] level filtering" << std::endl;
}

int main() {
    std::cout << "=== RoboLog 单元测试 ===" << std::endl;

    test_init_shutdown();
    test_log_levels();
    test_formatted_logging();
    test_file_output();
    test_level_filtering();

    std::cout << "\n=== 所有测试通过 ===" << std::endl;
    return 0;
}
