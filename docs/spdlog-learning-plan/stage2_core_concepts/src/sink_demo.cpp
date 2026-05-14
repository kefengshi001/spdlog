// spdlog 阶段 2: Sink 概念演示
// 演示各类内置 Sink 的使用

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/null_sink.h"

#include <iostream>
#include <memory>

void console_sink_demo() {
    std::cout << "\n=== 1. 控制台 Sink ===" << std::endl;

    auto console = spdlog::stdout_color_mt("console");
    console->info("stdout_color_sink: 带颜色的控制台输出");

    auto stderr_logger = spdlog::stderr_color_mt("stderr");
    stderr_logger->info("stderr_color_sink: 标准错误输出");

    spdlog::drop("console");
    spdlog::drop("stderr");
}

void file_sink_demo() {
    std::cout << "\n=== 2. 文件 Sink ===" << std::endl;

    // 基础文件 Sink
    auto basic = spdlog::basic_logger_mt("basic_file", "logs/basic.log", true);
    basic->info("basic_file_sink: 基础文件输出");

    // 轮转文件 Sink
    // 参数: 文件名, 最大大小(字节), 备份文件数
    auto rotating = spdlog::rotating_logger_mt("rotating", "logs/rotating.log",
        1048576 * 5, 3);  // 5MB, 3 backups
    rotating->info("rotating_file_sink: 按大小轮转");

    // 每日文件 Sink
    // 参数: 文件名, 小时, 分钟
    auto daily = spdlog::daily_logger_mt("daily", "logs/daily.log", 2, 30);
    daily->info("daily_file_sink: 每日轮转");

    spdlog::drop("basic_file");
    spdlog::drop("rotating");
    spdlog::drop("daily");
}

void null_sink_demo() {
    std::cout << "\n=== 3. 空 Sink ===" << std::endl;

    // 空 Sink 丢弃所有日志，用于测试
    auto null_logger = spdlog::create<spdlog::sinks::null_sink_mt>("null");
    null_logger->info("This message is discarded");
    std::cout << "null_sink: 消息被丢弃（无输出）" << std::endl;

    spdlog::drop("null");
}

void multi_sink_with_levels_demo() {
    std::cout << "\n=== 4. 多 Sink + 级别过滤 ===" << std::endl;

    // 控制台: 只显示 warn 及以上
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::warn);

    // 文件: 记录所有级别
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/all_levels.log", true);
    file_sink->set_level(spdlog::level::trace);

    // 错误文件: 只记录 error 及以上
    auto error_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/errors.log", true);
    error_sink->set_level(spdlog::level::err);

    spdlog::logger logger("multi_level", {console_sink, file_sink, error_sink});
    logger.set_level(spdlog::level::trace);

    logger.trace("trace: only in all_levels.log");
    logger.debug("debug: only in all_levels.log");
    logger.info("info: only in all_levels.log");
    logger.warn("warn: in all_levels.log + console");
    logger.error("error: in all_levels.log + errors.log + console");
    logger.critical("critical: in all_levels.log + errors.log + console");

    spdlog::drop("multi_level");
}

int main() {
    try {
        std::cout << "=== Sink 概念演示 ===" << std::endl;

        console_sink_demo();
        file_sink_demo();
        null_sink_demo();
        multi_sink_with_levels_demo();

        std::cout << "\n=== Sink 演示完成 ===" << std::endl;
        spdlog::shutdown();

    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
