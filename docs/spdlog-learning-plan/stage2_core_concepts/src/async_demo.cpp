// spdlog 阶段 2: 异步日志演示
// 演示同步 vs 异步日志的区别

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"

#include <iostream>
#include <chrono>

void sync_logger_demo() {
    std::cout << "\n=== 1. 同步日志 ===" << std::endl;

    auto start = std::chrono::steady_clock::now();

    auto sync_logger = spdlog::basic_logger_mt("sync", "logs/sync.log", true);
    for (int i = 0; i < 100; i++) {
        sync_logger->info("Sync message #{}", i);
    }
    sync_logger->flush();

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Sync logging: " << duration.count() << " microseconds" << std::endl;
    spdlog::drop("sync");
}

void async_logger_demo() {
    std::cout << "\n=== 2. 异步日志 ===" << std::endl;

    // 初始化异步日志线程池
    // 参数: 队列大小, 工作线程数
    spdlog::init_thread_pool(8192, 1);

    auto start = std::chrono::steady_clock::now();

    auto async_logger = spdlog::basic_logger_mt<spdlog::async_factory>(
        "async", "logs/async.log", true);

    for (int i = 0; i < 100; i++) {
        async_logger->info("Async message #{}", i);
    }
    async_logger->flush();

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Async logging: " << duration.count() << " microseconds" << std::endl;
    spdlog::drop("async");
}

void async_config_demo() {
    std::cout << "\n=== 3. 异步日志配置 ===" << std::endl;

    // 队列满时的行为
    // spdlog::async_overflow_policy::block_retry  - 阻塞等待（默认）
    // spdlog::async_overflow_policy::discard_log  - 丢弃新消息
    // spdlog::async_overflow_policy::overrun_oldest - 覆盖最旧消息

    std::cout << "异步日志配置参数:" << std::endl;
    std::cout << "  队列大小: 影响缓冲能力" << std::endl;
    std::cout << "  线程数: 影响并发写入能力" << std::endl;
    std::cout << "  溢出策略: 队列满时的行为" << std::endl;
}

int main() {
    try {
        std::cout << "=== 异步日志演示 ===" << std::endl;

        sync_logger_demo();
        async_logger_demo();
        async_config_demo();

        std::cout << "\n=== 异步日志演示完成 ===" << std::endl;
        std::cout << "注意: 异步日志适合高频场景（如 1kHz 传感器数据）" << std::endl;
        std::cout << "同步日志适合关键信息（如控制指令、错误报警）" << std::endl;

        spdlog::shutdown();

    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
