// RoboLog 配置文件

#pragma once

#include "robo_log.h"

// 预设配置

// 调试模式配置
inline RoboLogConfig debug_config() {
    RoboLogConfig config;
    config.level = spdlog::level::debug;
    config.console_enabled = true;
    config.file_enabled = true;
    config.error_file_enabled = true;
    config.async_enabled = false;  // 同步模式，便于调试
    return config;
}

// 生产模式配置
inline RoboLogConfig production_config() {
    RoboLogConfig config;
    config.level = spdlog::level::info;
    config.console_enabled = false;  // 生产环境不输出到控制台
    config.file_enabled = true;
    config.error_file_enabled = true;
    config.async_enabled = true;
    config.async_queue_size = 16384;
    config.async_thread_count = 1;
    return config;
}

// 高性能模式配置（用于高频数据记录）
inline RoboLogConfig high_performance_config() {
    RoboLogConfig config;
    config.level = spdlog::level::trace;
    config.console_enabled = false;
    config.file_enabled = true;
    config.max_file_size = 50 * 1024 * 1024;  // 50MB
    config.max_files = 10;
    config.error_file_enabled = true;
    config.async_enabled = true;
    config.async_queue_size = 65536;
    config.async_thread_count = 2;
    return config;
}
