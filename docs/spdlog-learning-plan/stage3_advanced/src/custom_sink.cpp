/**
 * spdlog 阶段 3: 自定义 Sink 开发 (Custom Sink Development)
 *
 * 本文件演示如何开发自定义 Sink，将日志消息发送到任意目标:
 *   1. callback_sink    - 通过回调函数处理日志(最灵活)
 *   2. ringbuffer_sink  - 环形缓冲区，保留最近 N 条日志
 *
 * 自定义 Sink 的核心步骤:
 *   1. 继承 spdlog::sinks::base_sink<mutex_type>
 *   2. 实现 sink_it_() 方法 — 定义如何处理每条日志
 *   3. 实现 flush_() 方法 — 定义如何刷新缓冲(可为空)
 *
 * base_sink<mutex_type> 的模板参数:
 *   - std::mutex:       线程安全(_mt 后缀)，推荐生产环境使用
 *   - spdlog::details::null_mutex: 非线程安全(_st 后缀)，单线程场景更快
 *
 * base_sink 已经提供了:
 *   - formatter_: 成员变量，存储格式化器，通过 formatter_->format(msg, dest) 使用
 *   - mutex_:     成员变量，自动在 sink_it_ 调用前加锁
 */

#include "spdlog/spdlog.h"
#include "spdlog/sinks/base_sink.h"      // 自定义 Sink 的基类
#include "spdlog/sinks/stdout_color_sinks.h"

#include <iostream>
#include <mutex>
#include <vector>
#include <deque>
#include <functional>

/**
 * 自定义 Sink 1: 回调函数 Sink (Callback Sink)
 *
 * 将格式化后的日志消息传递给用户提供的回调函数
 *
 * 应用场景:
 *   - 将日志转发到自定义日志收集系统(如 syslog、journald)
 *   - 在日志写入时触发告警(如 ERROR 级别时发送通知)
 *   - 将日志推送到消息队列(如 Kafka、RabbitMQ)
 *   - 单元测试中捕获日志进行断言
 *
 * 线程安全: 使用 std::mutex 模板参数，多线程调用 callback 是安全的
 */
class callback_sink : public spdlog::sinks::base_sink<std::mutex> {
public:
    // 回调函数类型: 接收格式化后的日志字符串
    using callback_t = std::function<void(const std::string &)>;

    explicit callback_sink(callback_t callback)
        : callback_(std::move(callback)) {}

protected:
    /**
     * sink_it_(): 核心方法，每条日志都会调用
     *
     * 调用时 base_sink 已经自动加锁，无需手动管理 mutex
     *
     * @param msg  日志消息结构体，包含:
     *   - msg.time:         时间戳 (std::chrono::system_clock::time_point)
     *   - msg.level:        日志级别 (trace/debug/info/warn/error/critical)
     *   - msg.logger_name:  logger 名称
     *   - msg.payload:      用户传入的原始消息(fmt::memory_buffer)
     *   - msg.thread_id:    线程 ID
     *   - msg.source:       源码位置(文件名、行号、函数名)
     */
    void sink_it_(const spdlog::details::log_msg &msg) override {
        spdlog::memory_buf_t formatted;
        // 使用 sink 自带的 formatter 将 log_msg 格式化为字符串
        // formatter 由 logger 创建时设置，默认为 "[%H:%M:%S] [%l] %v"
        formatter_->format(msg, formatted);
        // fmt::to_string() 将 memory_buffer 转换为 std::string
        callback_(fmt::to_string(formatted));
    }

    /**
     * flush_(): 刷新方法，当 logger->flush() 被调用时触发
     *
     * 对于回调 Sink，通常不需要特殊刷新逻辑
     * 但如果回调函数内部有缓冲区，应在此处刷新
     */
    void flush_() override {}

private:
    callback_t callback_;
};

/**
 * 自定义 Sink 2: 环形缓冲 Sink (Ring Buffer Sink)
 *
 * 在内存中保留最近 N 条格式化后的日志消息
 * 当缓冲区满时，最旧的消息会被丢弃(FIFO)
 *
 * 应用场景:
 *   - 崩溃日志收集: 平时不写文件，崩溃时导出最近 N 条日志
 *   - 调试监控: 实时查看最近的日志，无需文件 I/O
 *   - 内嵌设备: 内存有限，只保留关键日志
 *
 * 实现细节:
 *   - 使用 std::deque 作为底层容器(push_back + pop_front = O(1))
 *   - get_logs() 返回拷贝，避免外部修改内部状态
 */
class ringbuffer_sink : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit ringbuffer_sink(size_t max_size)
        : max_size_(max_size) {}

    /**
     * 获取当前缓冲区中的所有日志
     *
     * 注意: 返回的是拷贝，调用时会持有锁
     * 在高并发场景下，频繁调用可能影响日志写入性能
     */
    std::vector<std::string> get_logs() {
        std::lock_guard<std::mutex> lock(mutex_);  // 手动加锁保护 buffer_
        return {buffer_.begin(), buffer_.end()};
    }

protected:
    void sink_it_(const spdlog::details::log_msg &msg) override {
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        buffer_.push_back(fmt::to_string(formatted));

        // 超过最大容量时，移除最旧的消息
        // deque::pop_front() 是 O(1) 操作
        if (buffer_.size() > max_size_) {
            buffer_.pop_front();
        }
    }

    void flush_() override {}

private:
    size_t max_size_;
    std::deque<std::string> buffer_;
};

/**
 * 演示 1: 回调 Sink
 *
 * 创建一个将日志输出到 stdout 的回调 Sink
 * 实际应用中，回调可以是任何函数/Lambda/仿函数
 */
void callback_sink_demo() {
    std::cout << "\n=== 1. 回调 Sink ===" << std::endl;

    // 创建回调 Sink，Lambda 作为回调函数
    auto sink = std::make_shared<callback_sink>(
        [](const std::string &msg) {
            std::cout << "  [Callback] " << msg;  // msg 已包含换行符
        });

    // 手动创建 logger，传入自定义 sink 列表
    // spdlog::logger 构造函数: (name, sinks_init_list)
    spdlog::logger logger("callback", spdlog::sinks_init_list{sink});
    logger.info("This message goes to callback");
    logger.warn("Warning also goes to callback");
}

/**
 * 演示 2: 环形缓冲 Sink
 *
 * 创建容量为 5 的环形缓冲，写入 10 条日志
 * 验证只有最后 5 条被保留
 */
void ringbuffer_sink_demo() {
    std::cout << "\n=== 2. 环形缓冲 Sink ===" << std::endl;

    auto sink = std::make_shared<ringbuffer_sink>(5);  // 最多保留 5 条

    spdlog::logger logger("ringbuffer", spdlog::sinks_init_list{sink});
    // 写入 10 条日志，前 5 条会被覆盖
    for (int i = 0; i < 10; i++) {
        logger.info("Message #{}", i);
    }

    // 获取并显示缓冲区中的日志(应为 #5 ~ #9)
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
