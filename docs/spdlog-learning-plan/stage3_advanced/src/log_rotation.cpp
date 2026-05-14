// spdlog 阶段 3: 日志轮转策略
// 演示按大小、按时间、组合轮转策略

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/hourly_file_sink.h"

#include <iostream>

void rotating_by_size_demo() {
    std::cout << "\n=== 1. 按大小轮转 ===" << std::endl;

    // 参数: 文件名, 最大大小(字节), 备份文件数
    auto logger = spdlog::rotating_logger_mt(
        "rotating_size", "logs/rotating_size.log",
        1024 * 1024,  // 1MB
        3);            // 3 个备份

    logger->info("Rotating by size: max 1MB, 3 backups");
    logger->info("Files: rotating_size.log, rotating_size.log.1, .2, .3");

    spdlog::drop("rotating_size");
}

void rotating_by_time_demo() {
    std::cout << "\n=== 2. 按时间轮转 ===" << std::endl;

    // 每日轮转: 每天凌晨 2:30 创建新文件
    auto daily_logger = spdlog::daily_logger_mt(
        "daily", "logs/daily.log", 2, 30);

    daily_logger->info("Daily rotation: new file at 02:30 every day");

    // 每小时轮转
    auto hourly_logger = spdlog::hourly_logger_mt(
        "hourly", "logs/hourly.log");

    hourly_logger->info("Hourly rotation: new file every hour");

    spdlog::drop("daily");
    spdlog::drop("hourly");
}

void rotation_strategy_demo() {
    std::cout << "\n=== 3. 轮转策略选择 ===" << std::endl;

    std::cout << "  按大小轮转:" << std::endl;
    std::cout << "    - 适用于: 固定大小的日志文件" << std::endl;
    std::cout << "    - 优点: 磁盘空间可控" << std::endl;
    std::cout << "    - 缺点: 时间信息不明显" << std::endl;

    std::cout << "  按时间轮转:" << std::endl;
    std::cout << "    - 适用于: 按日期分割的日志" << std::endl;
    std::cout << "    - 优点: 便于按时间查找" << std::endl;
    std::cout << "    - 缺点: 文件大小不可控" << std::endl;

    std::cout << "  组合策略:" << std::endl;
    std::cout << "    - 按时间轮转 + 按大小限制" << std::endl;
    std::cout << "    - 需要自定义实现" << std::endl;
}

int main() {
    try {
        std::cout << "=== 日志轮转策略演示 ===" << std::endl;

        rotating_by_size_demo();
        rotating_by_time_demo();
        rotation_strategy_demo();

        std::cout << "\n=== 日志轮转策略演示完成 ===" << std::endl;
        spdlog::shutdown();

    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
