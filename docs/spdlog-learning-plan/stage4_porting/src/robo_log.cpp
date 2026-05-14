// RoboLog 实现

#include "robo_log.h"

std::shared_ptr<spdlog::logger> RoboLog::logger_ = nullptr;
bool RoboLog::initialized_ = false;

void RoboLog::init(const RoboLogConfig &config) {
    if (initialized_) {
        return;
    }

    std::vector<spdlog::sink_ptr> sinks;

    // 控制台 Sink
    if (config.console_enabled) {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::warn);
        console_sink->set_pattern("[%H:%M:%S] [%^%l%$] %v");
        sinks.push_back(console_sink);
    }

    // 运行日志文件 Sink
    if (config.file_enabled) {
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            config.file_path, config.max_file_size, config.max_files);
        file_sink->set_level(spdlog::level::trace);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%-8l%$] [%t] %v");
        sinks.push_back(file_sink);
    }

    // 错误日志文件 Sink
    if (config.error_file_enabled) {
        auto error_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            config.error_file_path, true);
        error_sink->set_level(spdlog::level::err);
        error_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%-8l%$] [%t] %v");
        sinks.push_back(error_sink);
    }

    // 创建 Logger
    if (config.async_enabled) {
        spdlog::init_thread_pool(config.async_queue_size, config.async_thread_count);
        logger_ = std::make_shared<spdlog::logger>("robo_log", sinks.begin(), sinks.end());
    } else {
        logger_ = std::make_shared<spdlog::logger>("robo_log", sinks.begin(), sinks.end());
    }

    logger_->set_level(config.level);
    spdlog::set_default_logger(logger_);

    initialized_ = true;
}

void RoboLog::shutdown() {
    if (logger_) {
        logger_->flush();
    }
    spdlog::shutdown();
    logger_.reset();
    initialized_ = false;
}

void RoboLog::set_level(spdlog::level::level_enum level) {
    if (logger_) {
        logger_->set_level(level);
    }
}

void RoboLog::flush() {
    if (logger_) {
        logger_->flush();
    }
}

void RoboLog::trace(const std::string &msg) {
    if (logger_) logger_->trace(msg);
}

void RoboLog::debug(const std::string &msg) {
    if (logger_) logger_->debug(msg);
}

void RoboLog::info(const std::string &msg) {
    if (logger_) logger_->info(msg);
}

void RoboLog::warn(const std::string &msg) {
    if (logger_) logger_->warn(msg);
}

void RoboLog::error(const std::string &msg) {
    if (logger_) logger_->error(msg);
}

void RoboLog::critical(const std::string &msg) {
    if (logger_) logger_->critical(msg);
}
