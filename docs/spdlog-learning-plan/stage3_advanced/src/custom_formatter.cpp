/**
 * spdlog 阶段 3: 自定义 Formatter 开发 (Custom Formatter Development)
 *
 * 本文件演示如何开发自定义 Formatter，将日志消息格式化为特定格式:
 *   1. JSON Formatter - 输出 JSON 格式，便于日志聚合系统(ELK、Loki)解析
 *   2. CSV Formatter  - 输出 CSV 格式，便于 Excel/Pandas 等工具分析
 *
 * 自定义 Formatter 的核心步骤:
 *   1. 继承 spdlog::formatter 基类
 *   2. 实现 format() 方法 — 将 log_msg 格式化写入 dest 缓冲区
 *   3. 实现 clone() 方法 — 返回自身的拷贝(formatter 需要可复制)
 *
 * log_msg 结构体关键字段:
 *   - time:         std::chrono::system_clock::time_point 时间戳
 *   - level:        spdlog::level::level_enum 日志级别
 *   - logger_name:  spdlog::string_view_t logger 名称
 *   - payload:      fmt::memory_buffer 用户消息(未格式化)
 *   - thread_id:    std::thread::id 线程 ID
 *   - source:       source_loc 源码位置(文件名、行号)
 *
 * 格式化输出方式:
 *   - fmt::format_to(std::back_inserter(dest), format_str, args...)
 *   - dest 是 spdlog::memory_buf_t (即 fmt::memory_buffer)
 *   - 使用 back_inserter 追加内容，不要覆盖已有内容
 */

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/formatter.h"  // formatter 基类定义

#include <iostream>
#include <ctime>
#include <chrono>

/**
 * 自定义 Formatter 1: JSON 格式
 *
 * 输出示例:
 *   {"timestamp":"2025-01-15T10:30:45.123","level":"info","logger":"mylog","message":"hello"}
 *
 * 应用场景:
 *   - ELK (Elasticsearch + Logstash + Kibana) 日志分析
 *   - Loki + Grafana 日志监控
 *   - 结构化日志收集系统
 *   - 日志通过 JSON 解析后可按字段检索和过滤
 *
 * 实现要点:
 *   - 使用 R"(...)" 原始字符串避免转义地狱
 *   - 时间格式化为 ISO 8601 (YYYY-MM-DDTHH:MM:SS.mmm)
 *   - 手动提取毫秒部分(chrono 只提供到秒的 to_time_t)
 */
class json_formatter : public spdlog::formatter {
public:
    void format(const spdlog::details::log_msg &msg, spdlog::memory_buf_t &dest) override {
        // 将 time_point 转换为 time_t (秒精度)
        auto time_point = msg.time;
        auto time_t_val = std::chrono::system_clock::to_time_t(time_point);
        auto time = *std::localtime(&time_t_val);  // 转换为本地时间结构体

        // 提取毫秒部分: 从 epoch 开始的总毫秒数 % 1000
        auto duration = time_point.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

        // 构建 JSON 字符串
        // 使用 fmt::format_to 写入 dest 缓冲区
        // R"(...)" 是 C++ 原始字符串字面量，不需要转义引号
        fmt::format_to(std::back_inserter(dest),
            R"({{"timestamp":"{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}.{:03d}","level":"{}","logger":"{}","message":"{}"}})",
            time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,  // tm_year 从 1900 起算
            time.tm_hour, time.tm_min, time.tm_sec,
            millis,
            spdlog::level::to_string_view(msg.level),  // 级别名称: "info", "warn" 等
            msg.logger_name,                              // logger 名称
            fmt::to_string(msg.payload));                 // 用户消息(payload 是 memory_buffer)
    }

    /**
     * clone(): 返回 formatter 的副本
     *
     * spdlog 内部需要复制 formatter(如创建新 sink 时)
     * 必须返回正确的派生类实例，否则会丢失自定义格式
     */
    std::unique_ptr<formatter> clone() const override {
        return std::make_unique<json_formatter>();
    }
};

/**
 * 自定义 Formatter 2: CSV 格式
 *
 * 输出示例:
 *   2025-01-15 10:30:45,info,mylog,hello
 *   2025-01-15 10:30:45,warn,mylog,warning
 *
 * 应用场景:
 *   - 用 Excel/WPS 打开分析日志
 *   - Python pandas 读取处理
 *   - 简单的文本日志分析
 *
 * 实现要点:
 *   - 字段间用逗号分隔
 *   - 每行以换行符结束
 *   - 注意: 如果消息本身包含逗号，CSV 解析可能出问题
 *     生产环境建议使用 json_formatter 或对字段做转义
 */
class csv_formatter : public spdlog::formatter {
public:
    void format(const spdlog::details::log_msg &msg, spdlog::memory_buf_t &dest) override {
        auto time_point = msg.time;
        auto time_t_val = std::chrono::system_clock::to_time_t(time_point);
        auto time = *std::localtime(&time_t_val);

        // CSV 格式: 时间,级别,logger名,消息
        fmt::format_to(std::back_inserter(dest),
            "{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d},{},{},{}\n",
            time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
            time.tm_hour, time.tm_min, time.tm_sec,
            spdlog::level::to_string_view(msg.level),
            msg.logger_name,
            fmt::to_string(msg.payload));
    }

    std::unique_ptr<formatter> clone() const override {
        return std::make_unique<csv_formatter>();
    }
};

/**
 * 演示 1: JSON Formatter
 *
 * 创建自定义 formatter 并应用到 sink:
 *   sink->set_formatter(std::make_unique<json_formatter>())
 *
 * 注意: formatter 是设置在 sink 上的，不是 logger 上
 *       同一个 logger 的不同 sink 可以使用不同的 formatter
 */
void json_formatter_demo() {
    std::cout << "\n=== 1. JSON Formatter ===" << std::endl;

    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    // 将自定义 JSON formatter 设置到 sink
    sink->set_formatter(std::make_unique<json_formatter>());

    // 使用配置好 formatter 的 sink 创建 logger
    spdlog::logger logger("json_logger", spdlog::sinks_init_list{sink});
    logger.info("JSON format message");
    logger.warn("JSON warning message");
    logger.error("JSON error message");
}

/**
 * 演示 2: CSV Formatter
 *
 * 同样创建 formatter 并设置到 sink
 */
void csv_formatter_demo() {
    std::cout << "\n=== 2. CSV Formatter ===" << std::endl;

    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sink->set_formatter(std::make_unique<csv_formatter>());

    spdlog::logger logger("csv_logger", spdlog::sinks_init_list{sink});
    logger.info("CSV format message");
    logger.warn("CSV warning message");
}

int main() {
    try {
        std::cout << "=== 自定义 Formatter 演示 ===" << std::endl;

        json_formatter_demo();
        csv_formatter_demo();

        std::cout << "\n=== 自定义 Formatter 演示完成 ===" << std::endl;
        spdlog::shutdown();

    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
