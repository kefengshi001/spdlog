// spdlog 阶段 1: 安装验证测试
// 验证 spdlog 编译、日志级别、文件输出是否正常

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#ifndef LOG_DIR
#define LOG_DIR "."
#endif

// Test 1: 基本日志输出不崩溃
void test_basic_logging() {
    spdlog::info("Test basic logging: info");
    spdlog::warn("Test basic logging: warn");
    spdlog::error("Test basic logging: error");
    std::cout << "  [PASS] basic logging" << std::endl;
}

// Test 2: 日志级别过滤
void test_log_levels() {
    auto logger = spdlog::stdout_color_mt("level_test");
    logger->set_level(spdlog::level::warn);

    // trace 和 debug 不应输出（但不应崩溃）
    logger->trace("This should not appear");
    logger->debug("This should not appear");
    logger->info("This should not appear");
    logger->warn("This should appear");
    logger->error("This should appear");

    spdlog::drop("level_test");
    std::cout << "  [PASS] log levels" << std::endl;
}

// Test 3: 文件输出
void test_file_output() {
    std::string log_path = std::string(LOG_DIR) + "/test_install.log";
    auto file_logger = spdlog::basic_logger_mt("file_test", log_path, true);
    file_logger->info("File output test message");
    file_logger->flush();
    spdlog::drop("file_test");

    // 验证文件存在且有内容
    std::ifstream file(log_path);
    assert(file.is_open());

    std::string content;
    std::getline(file, content);
    assert(!content.empty());
    assert(content.find("File output test message") != std::string::npos);

    file.close();
    std::cout << "  [PASS] file output" << std::endl;
}

// Test 4: 格式化输出
void test_formatting() {
    auto logger = spdlog::stdout_color_mt("format_test");
    logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
    logger->info("Formatted message with number: {}", 42);
    logger->info("Formatted message with float: {:.2f}", 3.14159);

    spdlog::drop("format_test");
    std::cout << "  [PASS] formatting" << std::endl;
}

// Test 5: 多 Sink
void test_multi_sink() {
    std::string log_path = std::string(LOG_DIR) + "/test_multi.log";

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path, true);

    spdlog::logger logger("multi_test", {console_sink, file_sink});
    logger.info("Multi sink test message");
    logger.flush();

    // 验证文件有输出
    std::ifstream file(log_path);
    assert(file.is_open());

    std::string content;
    std::getline(file, content);
    assert(content.find("Multi sink test message") != std::string::npos);

    file.close();
    std::cout << "  [PASS] multi sink" << std::endl;
}

int main() {
    std::cout << "=== spdlog 安装验证测试 ===" << std::endl;

    test_basic_logging();
    test_log_levels();
    test_file_output();
    test_formatting();
    test_multi_sink();

    spdlog::shutdown();
    std::cout << "\n=== 所有测试通过 ===" << std::endl;
    return 0;
}
