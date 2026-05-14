// RoboLog: 机械臂控制系统专用日志接口
// 封装 spdlog，提供简化的日志接口

#pragma once

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/async.h"

#include <memory>
#include <string>

// 日志配置
struct RoboLogConfig {
    spdlog::level::level_enum level = spdlog::level::info;
    bool console_enabled = true;
    bool file_enabled = true;
    std::string file_path = "logs/robot.log";
    size_t max_file_size = 10 * 1024 * 1024;  // 10MB
    size_t max_files = 5;
    bool error_file_enabled = true;
    std::string error_file_path = "logs/robot_error.log";
    bool async_enabled = true;
    size_t async_queue_size = 8192;
    size_t async_thread_count = 1;
};

class RoboLog {
public:
    // 初始化日志系统
    static void init(const RoboLogConfig &config = RoboLogConfig());

    // 关闭日志系统
    static void shutdown();

    // 设置日志级别
    static void set_level(spdlog::level::level_enum level);

    // 刷新日志
    static void flush();

    // 基本日志接口
    static void trace(const std::string &msg);
    static void debug(const std::string &msg);
    static void info(const std::string &msg);
    static void warn(const std::string &msg);
    static void error(const std::string &msg);
    static void critical(const std::string &msg);

    // 格式化日志接口
    template<typename... Args>
    static void trace(fmt::format_string<Args...> fmt, Args &&...args) {
        if (logger_) logger_->trace(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void debug(fmt::format_string<Args...> fmt, Args &&...args) {
        if (logger_) logger_->debug(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void info(fmt::format_string<Args...> fmt, Args &&...args) {
        if (logger_) logger_->info(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void warn(fmt::format_string<Args...> fmt, Args &&...args) {
        if (logger_) logger_->warn(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error(fmt::format_string<Args...> fmt, Args &&...args) {
        if (logger_) logger_->error(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void critical(fmt::format_string<Args...> fmt, Args &&...args) {
        if (logger_) logger_->critical(fmt, std::forward<Args>(args)...);
    }

private:
    static std::shared_ptr<spdlog::logger> logger_;
    static bool initialized_;
};
