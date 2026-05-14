// spdlog 阶段 3: 多线程安全日志实践
// 演示多线程环境下安全使用日志

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

void thread_safe_logger_demo() {
    std::cout << "\n=== 1. 线程安全 Logger ===" << std::endl;

    // 使用 _mt 后缀创建线程安全的 Logger
    auto logger = spdlog::basic_logger_mt("thread_safe", "logs/thread_safe.log", true);
    logger->set_level(spdlog::level::debug);

    const int num_threads = 4;
    const int messages_per_thread = 100;

    std::vector<std::thread> threads;
    std::atomic<int> ready{0};

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&logger, i, messages_per_thread, &ready]() {
            ready++;
            while (ready < num_threads) {}  // 等待所有线程就绪

            for (int j = 0; j < messages_per_thread; j++) {
                logger->info("Thread {} - Message {}", i, j);
            }
        });
    }

    for (auto &t : threads) {
        t.join();
    }

    logger->flush();
    std::cout << "  " << num_threads << " threads, "
              << messages_per_thread << " messages each" << std::endl;

    spdlog::drop("thread_safe");
}

void global_logger_thread_safety() {
    std::cout << "\n=== 2. 全局 Logger 线程安全 ===" << std::endl;

    auto global_logger = spdlog::basic_logger_mt("global", "logs/global.log", true);
    spdlog::set_default_logger(global_logger);

    const int num_threads = 4;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([i]() {
            for (int j = 0; j < 50; j++) {
                spdlog::info("Global logger - Thread {} - Message {}", i, j);
            }
        });
    }

    for (auto &t : threads) {
        t.join();
    }

    spdlog::shutdown();
}

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

        thread_safe_logger_demo();
        global_logger_thread_safety();
        thread_safety_best_practices();

        std::cout << "\n=== 多线程安全日志演示完成 ===" << std::endl;

    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
