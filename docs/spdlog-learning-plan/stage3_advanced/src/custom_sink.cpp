// spdlog 阶段 3: 自定义 Sink 开发
// 演示如何开发自定义 Sink

#include "spdlog/spdlog.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <iostream>
#include <mutex>
#include <vector>
#include <deque>
#include <functional>

// 自定义 Sink: 回调函数 Sink
// 将日志消息发送到自定义回调函数
class callback_sink : public spdlog::sinks::base_sink<std::mutex> {
public:
    using callback_t = std::function<void(const std::string &)>;

    explicit callback_sink(callback_t callback)
        : callback_(std::move(callback)) {}

protected:
    void sink_it_(const spdlog::details::log_msg &msg) override {
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        callback_(fmt::to_string(formatted));
    }

    void flush_() override {}

private:
    callback_t callback_;
};

// 自定义 Sink: 环形缓冲 Sink
// 保留最近 N 条日志
class ringbuffer_sink : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit ringbuffer_sink(size_t max_size)
        : max_size_(max_size) {}

    std::vector<std::string> get_logs() {
        std::lock_guard<std::mutex> lock(mutex_);
        return {buffer_.begin(), buffer_.end()};
    }

protected:
    void sink_it_(const spdlog::details::log_msg &msg) override {
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        buffer_.push_back(fmt::to_string(formatted));
        if (buffer_.size() > max_size_) {
            buffer_.pop_front();
        }
    }

    void flush_() override {}

private:
    size_t max_size_;
    std::deque<std::string> buffer_;
};

void callback_sink_demo() {
    std::cout << "\n=== 1. 回调 Sink ===" << std::endl;

    auto sink = std::make_shared<callback_sink>(
        [](const std::string &msg) {
            std::cout << "  [Callback] " << msg;
        });

    spdlog::logger logger("callback", spdlog::sinks_init_list{sink});
    logger.info("This message goes to callback");
    logger.warn("Warning also goes to callback");
}

void ringbuffer_sink_demo() {
    std::cout << "\n=== 2. 环形缓冲 Sink ===" << std::endl;

    auto sink = std::make_shared<ringbuffer_sink>(5);

    spdlog::logger logger("ringbuffer", spdlog::sinks_init_list{sink});
    for (int i = 0; i < 10; i++) {
        logger.info("Message #{}", i);
    }

    auto logs = sink->get_logs();
    std::cout << "  Buffer contains " << logs.size() << " messages:" << std::endl;
    for (const auto &log : logs) {
        std::cout << "    " << log;
    }
}

int main() {
    try {
        std::cout << "=== 自定义 Sink 演示 ===" << std::endl;

        callback_sink_demo();
        ringbuffer_sink_demo();

        std::cout << "\n=== 自定义 Sink 演示完成 ===" << std::endl;
        spdlog::shutdown();

    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
