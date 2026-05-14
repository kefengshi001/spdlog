// spdlog 阶段 3: 自定义 Formatter 开发
// 演示如何开发自定义 Formatter

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/formatter.h"

#include <iostream>
#include <ctime>
#include <chrono>

// 自定义 Formatter: JSON 格式
class json_formatter : public spdlog::formatter {
public:
    void format(const spdlog::details::log_msg &msg, spdlog::memory_buf_t &dest) override {
        // 格式化时间为 ISO 8601
        auto time_point = msg.time;
        auto time_t_val = std::chrono::system_clock::to_time_t(time_point);
        auto time = *std::localtime(&time_t_val);

        // 构建 JSON 字符串
        auto duration = time_point.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
        fmt::format_to(std::back_inserter(dest),
            R"({{"timestamp":"{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}.{:03d}","level":"{}","logger":"{}","message":"{}"}})",
            time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
            time.tm_hour, time.tm_min, time.tm_sec,
            millis,
            spdlog::level::to_string_view(msg.level),
            msg.logger_name,
            fmt::to_string(msg.payload));
    }

    std::unique_ptr<formatter> clone() const override {
        return std::make_unique<json_formatter>();
    }
};

// 自定义 Formatter: CSV 格式
class csv_formatter : public spdlog::formatter {
public:
    void format(const spdlog::details::log_msg &msg, spdlog::memory_buf_t &dest) override {
        auto time_point = msg.time;
        auto time_t_val = std::chrono::system_clock::to_time_t(time_point);
        auto time = *std::localtime(&time_t_val);

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

void json_formatter_demo() {
    std::cout << "\n=== 1. JSON Formatter ===" << std::endl;

    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sink->set_formatter(std::make_unique<json_formatter>());

    spdlog::logger logger("json_logger", spdlog::sinks_init_list{sink});
    logger.info("JSON format message");
    logger.warn("JSON warning message");
    logger.error("JSON error message");
}

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
