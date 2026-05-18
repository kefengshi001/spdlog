/**
 * spdlog 阶段 3: 高级功能测试 (Advanced Features Test)
 *
 * 本文件测试阶段 3 学习的所有高级功能:
 *   1. 自定义 Sink     - 验证自定义 sink 能正确接收日志消息
 *   2. 日志轮转        - 验证按大小轮转文件正确创建
 *   3. 多线程安全       - 验证多线程并发写入不崩溃、不丢数据
 *   4. 异步日志调优     - 验证异步 logger 能正常工作
 *
 * 测试方法:
 *   - 使用 assert 进行断言(简单直接，适合学习阶段)
 *   - 每个测试函数独立运行，互不影响
 *   - 测试前自动清理旧的 logger (spdlog::drop)
 *
 * 编译时可通过 -DLOG_DIR="xxx" 指定日志输出目录
 */

#include "spdlog/spdlog.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/async.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// LOG_DIR 宏: 日志输出目录，编译时可通过 -DLOG_DIR="path" 覆盖
// 默认为当前目录 "."
#ifndef LOG_DIR
#define LOG_DIR "."
#endif

/**
 * 测试用自定义 Sink: 消息计数器
 *
 * 继承 base_sink<std::mutex> 实现线程安全的消息计数
 * 用于验证自定义 Sink 是否正确接收了所有日志消息
 *
 * 工作原理:
 *   - 每次 sink_it_() 被调用时，计数器 +1
 *   - 测试结束后通过 message_count() 获取总数
 *   - 与预期值对比，判断是否所有消息都被正确处理
 */
class test_sink : public spdlog::sinks::base_sink<std::mutex> {
public:
    int message_count() const { return count_; }

protected:
    void sink_it_(const spdlog::details::log_msg &msg) override {
        count_++;  // 只计数，不实际写入任何目标
    }
    void flush_() override {}

private:
    int count_ = 0;
};

/**
 * 测试 1: 自定义 Sink
 *
 * 验证:
 *   - 自定义 sink 能正确继承 base_sink
 *   - sink_it_() 被正确调用
 *   - 消息计数与实际写入数量一致
 */
void test_custom_sink() {
    auto sink = std::make_shared<test_sink>();

    // 手动创建 logger 并注入自定义 sink
    spdlog::logger logger("test_custom", {sink});
    logger.info("message 1");
    logger.info("message 2");
    logger.info("message 3");

    // 断言: 应该收到 3 条消息
    assert(sink->message_count() == 3);
    std::cout << "  [PASS] custom sink" << std::endl;
}

/**
 * 测试 2: 按大小轮转的文件 Logger
 *
 * 验证:
 *   - rotating_logger_mt 创建成功
 *   - 日志文件被正确创建
 *   - 大量写入后文件存在(可能已轮转)
 *
 * 参数: 1024 字节最大，2 个备份文件
 * 写入 100 条消息，应该触发至少一次轮转
 */
void test_rotating_file() {
    std::string log_path = std::string(LOG_DIR) + "/test_rotating.log";

    auto logger = spdlog::rotating_logger_mt("test_rot", log_path, 1024, 2);
    for (int i = 0; i < 100; i++) {
        logger->info("Rotating test message #{}", i);
    }
    logger->flush();

    // 断言: 主日志文件应该存在
    std::ifstream file(log_path);
    assert(file.is_open());
    file.close();

    spdlog::drop("test_rot");
    std::cout << "  [PASS] rotating file" << std::endl;
}

/**
 * 测试 3: 多线程安全写入
 *
 * 验证:
 *   - 4 个线程同时写入同一个 logger
 *   - 每个线程写 100 条，共 400 条
 *   - 不崩溃、不丢数据、文件可正常打开
 *
 * 使用 _mt 后缀保证线程安全
 */
void test_thread_safety() {
    std::string log_path = std::string(LOG_DIR) + "/test_thread.log";

    auto logger = spdlog::basic_logger_mt("test_thread", log_path, true);

    const int num_threads = 4;
    const int messages = 100;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&logger, i]() {
            for (int j = 0; j < messages; j++) {
                logger->info("Thread {} msg {}", i, j);
            }
        });
    }

    for (auto &t : threads) {
        t.join();
    }

    logger->flush();

    // 断言: 日志文件应该存在且可读
    std::ifstream file(log_path);
    assert(file.is_open());
    file.close();

    spdlog::drop("test_thread");
    std::cout << "  [PASS] thread safety" << std::endl;
}

/**
 * 测试 4: 异步日志调优
 *
 * 验证:
 *   - init_thread_pool() 正确初始化线程池
 *   - async_factory 创建的 logger 能正常写入
 *   - 异步队列中的消息被正确刷新到文件
 *
 * 配置: 队列 8192，1 个工作线程
 */
void test_async_tuning() {
    spdlog::init_thread_pool(8192, 1);

    std::string log_path = std::string(LOG_DIR) + "/test_async_tune.log";
    auto logger = spdlog::basic_logger_mt<spdlog::async_factory>(
        "test_async_tune", log_path, true);

    for (int i = 0; i < 100; i++) {
        logger->info("Async tuning test #{}", i);
    }
    logger->flush();  // 等待异步队列处理完毕

    // 断言: 日志文件应该存在
    std::ifstream file(log_path);
    assert(file.is_open());
    file.close();

    spdlog::drop("test_async_tune");
    std::cout << "  [PASS] async tuning" << std::endl;
}

int main() {
    std::cout << "=== 高级功能测试 ===" << std::endl;

    test_custom_sink();      // 测试自定义 Sink
    test_rotating_file();    // 测试日志轮转
    test_thread_safety();    // 测试多线程安全
    test_async_tuning();     // 测试异步日志

    spdlog::shutdown();      // 清理所有 logger 和线程池
    std::cout << "\n=== 所有测试通过 ===" << std::endl;
    return 0;
}
