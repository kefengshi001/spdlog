// spdlog 阶段 2: 核心概念测试
// 验证 Logger、Sink、Formatter、异步日志功能

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/null_sink.h"
#include "spdlog/async.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#ifndef LOG_DIR
#define LOG_DIR "."
#endif

void test_logger_creation() {
    auto logger = spdlog::stdout_color_mt("test_logger");
    assert(logger != nullptr);
    assert(logger->name() == "test_logger");

    logger->info("Logger creation test");
    spdlog::drop("test_logger");
    std::cout << "  [PASS] logger creation" << std::endl;
}

void test_log_levels() {
    auto logger = spdlog::stdout_color_mt("test_levels");
    logger->set_level(spdlog::level::debug);

    // 不应崩溃
    logger->trace("trace");
    logger->debug("debug");
    logger->info("info");
    logger->warn("warn");
    logger->error("error");
    logger->critical("critical");

    spdlog::drop("test_levels");
    std::cout << "  [PASS] log levels" << std::endl;
}

void test_multi_sink() {
    std::string log_path = std::string(LOG_DIR) + "/test_multi_sink.log";

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path, true);

    spdlog::logger logger("test_multi", {console_sink, file_sink});
    logger.info("Multi sink test");
    logger.flush();

    std::ifstream file(log_path);
    assert(file.is_open());

    std::string content;
    std::getline(file, content);
    assert(content.find("Multi sink test") != std::string::npos);
    file.close();

    std::cout << "  [PASS] multi sink" << std::endl;
}

void test_sink_level_filter() {
    std::string log_path = std::string(LOG_DIR) + "/test_level_filter.log";

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path, true);
    file_sink->set_level(spdlog::level::warn);

    spdlog::logger logger("test_filter", {file_sink});
    logger.set_level(spdlog::level::trace);

    logger.debug("should not appear");
    logger.info("should not appear");
    logger.warn("should appear");
    logger.error("should appear");
    logger.flush();

    std::ifstream file(log_path);
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    assert(content.find("should not appear") == std::string::npos);
    assert(content.find("should appear") != std::string::npos);
    file.close();

    std::cout << "  [PASS] sink level filter" << std::endl;
}

void test_formatter() {
    auto logger = spdlog::stdout_color_mt("test_format");
    logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
    logger->info("Format test");

    logger->set_pattern("[%n] %v");
    logger->info("Logger name test");

    spdlog::set_pattern("%+");  // Reset
    spdlog::drop("test_format");
    std::cout << "  [PASS] formatter" << std::endl;
}

void test_rotating_file() {
    std::string log_path = std::string(LOG_DIR) + "/test_rotating.log";

    auto logger = spdlog::rotating_logger_mt("test_rotating", log_path, 1024, 2);
    for (int i = 0; i < 100; i++) {
        logger->info("Rotating test message #{}", i);
    }
    logger->flush();

    std::ifstream file(log_path);
    assert(file.is_open());
    file.close();

    spdlog::drop("test_rotating");
    std::cout << "  [PASS] rotating file" << std::endl;
}

void test_async_logger() {
    spdlog::init_thread_pool(8192, 1);

    std::string log_path = std::string(LOG_DIR) + "/test_async.log";
    auto logger = spdlog::basic_logger_mt<spdlog::async_factory>(
        "test_async", log_path, true);

    for (int i = 0; i < 50; i++) {
        logger->info("Async test #{}", i);
    }
    logger->flush();

    std::ifstream file(log_path);
    assert(file.is_open());
    file.close();

    spdlog::drop("test_async");
    std::cout << "  [PASS] async logger" << std::endl;
}

void test_null_sink() {
    auto logger = spdlog::create<spdlog::sinks::null_sink_mt>("test_null");
    logger->info("This should be discarded");
    // No crash = pass

    spdlog::drop("test_null");
    std::cout << "  [PASS] null sink" << std::endl;
}

int main() {
    std::cout << "=== 核心概念测试 ===" << std::endl;

    test_logger_creation();
    test_log_levels();
    test_multi_sink();
    test_sink_level_filter();
    test_formatter();
    test_rotating_file();
    test_async_logger();
    test_null_sink();

    spdlog::shutdown();
    std::cout << "\n=== 所有测试通过 ===" << std::endl;
    return 0;
}
