/**
 * spdlog 阶段 3: 多线程安全日志实践 (Thread-Safe Logging)
 *
 * 本文件演示多线程环境下安全使用 spdlog 的方法:
 *   1. 线程安全 Logger  - 使用 _mt 后缀创建线程安全的 logger
 *   2. 全局 Logger 安全 - spdlog::info() 等全局函数的线程安全性
 *   3. 最佳实践总结
 *
 * spdlog 线程安全的关键设计:
 *   - _mt (multi-threaded) 后缀: Logger 和 Sink 内部使用 std::mutex 保护
 *   - _st (single-threaded) 后缀: 无锁，单线程场景下性能更好
 *   - 异步日志天生线程安全: 生产者只写队列，消费者由专用线程处理
 *
 * 常见的多线程日志问题:
 *   - 日志交错: 不同线程的日志消息混在一起(字符级别交错)
 *   - 数据竞争: 多线程同时写同一个文件导致数据损坏
 *   - 死锁: 在日志回调中再次记录日志导致递归加锁
 *
 * spdlog 的 _mt Logger 通过 mutex 解决了前两个问题
 * 第三个问题需要用户避免在回调中记录日志
 */

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

/**
 * 演示 1: 线程安全 Logger
 *
 * 关键点:
 *   - basic_logger_mt: 创建线程安全的 logger (_mt = mutex protected)
 *   - 多个线程可以安全地同时调用同一个 logger 的 info/warn/error
 *   - spdlog 内部保证每条日志消息的原子性(不会出现字符交错)
 *
 * 测试方法:
 *   - 创建 4 个线程，每个线程写 100 条日志
 *   - 使用 atomic 计数器实现"同时起跑"(避免线程启动时间差)
 *   - 验证不会崩溃、不会死锁
 */
void thread_safe_logger_demo() {
    std::cout << "\n=== 1. 线程安全 Logger ===" << std::endl;

    // _mt 后缀: 内部使用 std::mutex 保护所有写操作
    auto logger = spdlog::basic_logger_mt("thread_safe", "logs/thread_safe.log", true);
    logger->set_level(spdlog::level::debug);

    const int num_threads = 4;
    const int messages_per_thread = 100;

    std::vector<std::thread> threads;
    std::atomic<int> ready{0};  // 原子计数器，用于同步线程启动

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&logger, i, messages_per_thread, &ready]() {
            ready++;  // 原子递增，标记本线程已就绪
            // 自旋等待所有线程就绪，确保同时开始写日志
            // 这样可以最大化并发冲突的可能性，验证线程安全性
            while (ready < num_threads) {}

            for (int j = 0; j < messages_per_thread; j++) {
                logger->info("Thread {} - Message {}", i, j);
            }
        });
    }

    // 等待所有线程完成
    for (auto &t : threads) {
        t.join();
    }

    logger->flush();
    std::cout << "  " << num_threads << " threads, "
              << messages_per_thread << " messages each" << std::endl;

    spdlog::drop("thread_safe");
}

/**
 * 演示 2: 全局 Logger 线程安全
 *
 * spdlog::info() / spdlog::warn() 等全局函数:
 *   - 内部调用 spdlog::default_logger() 获取全局 logger
 *   - 全局 logger 本身也是线程安全的
 *   - 适合在多线程程序中作为统一的日志入口
 *
 * 注意:
 *   - set_default_logger() 本身不是线程安全的
 *   - 应在程序初始化阶段(单线程)调用
 *   - 初始化完成后，多线程可以安全使用 spdlog::info()
 */
void global_logger_thread_safety() {
    std::cout << "\n=== 2. 全局 Logger 线程安全 ===" << std::endl;

    // 在主线程中设置全局 logger (初始化阶段，单线程)
    auto global_logger = spdlog::basic_logger_mt("global", "logs/global.log", true);
    spdlog::set_default_logger(global_logger);

    const int num_threads = 4;
    std::vector<std::thread> threads;

    // 多线程通过 spdlog::info() 全局函数写日志
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([i]() {
            for (int j = 0; j < 50; j++) {
                // spdlog::info() 是线程安全的
                // 内部会获取 default_logger 并加锁
                spdlog::info("Global logger - Thread {} - Message {}", i, j);
            }
        });
    }

    for (auto &t : threads) {
        t.join();
    }

    spdlog::shutdown();
}

/**
 * 演示 3: 多线程日志最佳实践
 *
 * 1. 使用 _mt 后缀的 Sink 和 Logger
 *    - basic_logger_mt, rotating_logger_mt 等
 *    - stdout_color_sink_mt, basic_file_sink_mt 等
 *
 * 2. 避免在日志回调中再次记录日志
 *    - 可能导致递归加锁 → 死锁
 *    - 如需在回调中输出，使用无锁的 stdout 直接输出
 *
 * 3. 使用 flush() 确保日志写入
 *    - 多线程程序结束前，必须 flush + shutdown
 *    - 否则队列中残留的日志会丢失
 *
 * 4. 异步日志天然线程安全
 *    - async_factory 创建的 logger 无需额外同步
 *    - 生产者(调用方)只写队列，消费者(后台线程)独占写入
 *
 * 5. 全局 Logger 使用 spdlog::set_default_logger()
 *    - 在 main() 开始时初始化一次
 *    - 之后所有线程通过 spdlog::info() 等函数使用
 */
void thread_safety_best_practices() {
    std::cout << "\n=== 3. 多线程最佳实践 ===" << std::endl;

    std::cout << "  1. 使用 _mt 后缀的 Sink 和 Logger" << std::endl;
    std::cout << "  2. 避免在日志回调中再次记录日志" << std::endl;
    std::cout << "  3. 使用 flush() 确保日志写入" << std::endl;
    std::cout << "  4. 异步日志天然线程安全" << std::endl;
    std::cout << "  5. 全局 Logger 使用 spdlog::set_default_logger()" << std::endl;
}

int main() {
    try {
        std::cout << "=== 多线程安全日志演示 ===" << std::endl;

        thread_safe_logger_demo();       // 专用 logger 多线程写入
        global_logger_thread_safety();   // 全局 logger 多线程写入
        thread_safety_best_practices();  // 最佳实践总结

        std::cout << "\n=== 多线程安全日志演示完成 ===" << std::endl;

    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
