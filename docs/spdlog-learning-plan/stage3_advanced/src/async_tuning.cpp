// spdlog 阶段 3: 异步日志调优
// 演示异步日志参数调优

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"

#include <iostream>
#include <chrono>
#include <vector>

void queue_size_demo() {
    std::cout << "\n=== 1. 队列大小调优 ===" << std::endl;

    // 较小队列: 内存占用少，但可能阻塞
    spdlog::init_thread_pool(1024, 1);
    auto small_queue = spdlog::basic_logger_mt<spdlog::async_factory>(
        "small_queue", "logs/small_queue.log", true);

    // 较大队列: 缓冲能力强，内存占用多
    // 注意: 需要先 shutdown 再重新初始化
    spdlog::shutdown();
    spdlog::init_thread_pool(65536, 1);
    auto large_queue = spdlog::basic_logger_mt<spdlog::async_factory>(
        "large_queue", "logs/large_queue.log", true);

    std::cout << "  小队列(1024): 适合低频日志" << std::endl;
    std::cout << "  大队列(65536): 适合高频日志" << std::endl;

    spdlog::drop("small_queue");
    spdlog::drop("large_queue");
}

void thread_count_demo() {
    std::cout << "\n=== 2. 线程数调优 ===" << std::endl;

    spdlog::shutdown();
    spdlog::init_thread_pool(8192, 2);  // 2 个工作线程

    auto logger = spdlog::basic_logger_mt<spdlog::async_factory>(
        "multi_thread", "logs/multi_thread.log", true);

    std::cout << "  2 个工作线程: 适合多文件写入场景" << std::endl;
    std::cout << "  1 个工作线程: 适合单文件写入场景" << std::endl;

    spdlog::drop("multi_thread");
}

void overflow_policy_demo() {
    std::cout << "\n=== 3. 溢出策略 ===" << std::endl;

    std::cout << "  block_retry: 阻塞等待（默认，最安全）" << std::endl;
    std::cout << "  discard_log: 丢弃新消息（最快）" << std::endl;
    std::cout << "  overrun_oldest: 覆盖最旧消息" << std::endl;

    // 示例: 使用丢弃策略
    spdlog::shutdown();
    spdlog::init_thread_pool(1024, 1);

    auto logger = spdlog::basic_logger_mt<spdlog::async_factory>(
        "discard", "logs/discard.log", true);

    // 快速写入大量日志，队列满时丢弃
    for (int i = 0; i < 10000; i++) {
        logger->info("Fast logging #{}", i);
    }

    spdlog::drop("discard");
}

void performance_comparison() {
    std::cout << "\n=== 4. 性能对比 ===" << std::endl;

    const int iterations = 10000;

    // 同步日志
    spdlog::shutdown();
    auto sync_logger = spdlog::basic_logger_mt("sync_perf", "logs/sync_perf.log", true);

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; i++) {
        sync_logger->info("Sync message #{}", i);
    }
    sync_logger->flush();
    auto end = std::chrono::steady_clock::now();
    auto sync_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // 异步日志
    spdlog::drop("sync_perf");
    spdlog::init_thread_pool(65536, 1);
    auto async_logger = spdlog::basic_logger_mt<spdlog::async_factory>(
        "async_perf", "logs/async_perf.log", true);

    start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; i++) {
        async_logger->info("Async message #{}", i);
    }
    async_logger->flush();
    end = std::chrono::steady_clock::now();
    auto async_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "  同步日志 (" << iterations << " 条): " << sync_time << "ms" << std::endl;
    std::cout << "  异步日志 (" << iterations << " 条): " << async_time << "ms" << std::endl;
    std::cout << "  异步日志快 " << (sync_time > 0 ? (sync_time - async_time) * 100 / sync_time : 0) << "%" << std::endl;

    spdlog::drop("async_perf");
}

int main() {
    try {
        std::cout << "=== 异步日志调优演示 ===" << std::endl;

        queue_size_demo();
        thread_count_demo();
        overflow_policy_demo();
        performance_comparison();

        std::cout << "\n=== 异步日志调优演示完成 ===" << std::endl;
        spdlog::shutdown();

    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
