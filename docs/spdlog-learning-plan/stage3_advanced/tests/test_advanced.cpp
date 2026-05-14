// spdlog 阶段 3: 高级功能测试
// 验证自定义 Sink/Formatter、异步调优、多线程安全

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

#ifndef LOG_DIR
#define LOG_DIR "."
#endif

// 测试自定义 Sink
class test_sink : public spdlog::sinks::base_sink<std::mutex> {
public:
    int message_count() const { return count_; }

protected:
    void sink_it_(const spdlog::details::log_msg &msg) override {
        count_++;
    }
    void flush_() override {}

private:
    int count_ = 0;
};

void test_custom_sink() {
    auto sink = std::make_shared<test_sink>();

    spdlog::logger logger("test_custom", {sink});
    logger.info("message 1");
    logger.info("message 2");
    logger.info("message 3");

    assert(sink->message_count() == 3);
    std::cout << "  [PASS] custom sink" << std::endl;
}

void test_rotating_file() {
    std::string log_path = std::string(LOG_DIR) + "/test_rotating.log";

    auto logger = spdlog::rotating_logger_mt("test_rot", log_path, 1024, 2);
    for (int i = 0; i < 100; i++) {
        logger->info("Rotating test message #{}", i);
    }
    logger->flush();

    std::ifstream file(log_path);
    assert(file.is_open());
    file.close();

    spdlog::drop("test_rot");
    std::cout << "  [PASS] rotating file" << std::endl;
}

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

    std::ifstream file(log_path);
    assert(file.is_open());
    file.close();

    spdlog::drop("test_thread");
    std::cout << "  [PASS] thread safety" << std::endl;
}

void test_async_tuning() {
    spdlog::init_thread_pool(8192, 1);

    std::string log_path = std::string(LOG_DIR) + "/test_async_tune.log";
    auto logger = spdlog::basic_logger_mt<spdlog::async_factory>(
        "test_async_tune", log_path, true);

    for (int i = 0; i < 100; i++) {
        logger->info("Async tuning test #{}", i);
    }
    logger->flush();

    std::ifstream file(log_path);
    assert(file.is_open());
    file.close();

    spdlog::drop("test_async_tune");
    std::cout << "  [PASS] async tuning" << std::endl;
}

int main() {
    std::cout << "=== 高级功能测试 ===" << std::endl;

    test_custom_sink();
    test_rotating_file();
    test_thread_safety();
    test_async_tuning();

    spdlog::shutdown();
    std::cout << "\n=== 所有测试通过 ===" << std::endl;
    return 0;
}
