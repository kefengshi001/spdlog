/**
 * RoboLog 实现 (RoboLog Implementation)
 *
 * RoboLog 是对 spdlog 的二次封装，专为机器人/嵌入式场景设计
 *
 * 设计目标:
 *   1. 简化接口: 隐藏 spdlog 的复杂配置，提供 init/shutdown + 级别函数
 *   2. 多 Sink 架构: 控制台(快速查看) + 运行日志文件(全量) + 错误文件(仅错误)
 *   3. 支持异步: 可选异步模式，避免日志 I/O 阻塞控制循环
 *   4. 线程安全: 使用 _mt 后缀的 sink，多线程可安全调用
 *
 * 架构:
 *   RoboLog (静态类)
 *       ├── init(config)      → 根据配置创建 sink 组合 + logger
 *       ├── shutdown()        → 刷新 + 清理
 *       ├── info/warn/error() → 转发到 spdlog logger
 *       └── logger_           → 底层 spdlog::logger 实例(单例)
 *
 * 日志流向:
 *   用户调用 RoboLog::info("msg")
 *       → logger_->info("msg")
 *           → sink1 (console):  输出到终端 [HH:MM:SS] [info] msg
 *           → sink2 (file):     写入运行日志 [2025-01-15 HH:MM:SS.mmm] [info    ] [tid] msg
 *           → sink3 (error):    (仅 error/critical) 写入错误日志
 */

#include "robo_log.h"

#include <filesystem>

// 静态成员初始化
std::shared_ptr<spdlog::logger> RoboLog::logger_ = nullptr;
bool RoboLog::initialized_ = false;

/**
 * 初始化 RoboLog
 *
 * 根据 RoboLogConfig 配置创建日志系统:
 *   1. 创建 Sink 组合(控制台 + 文件 + 错误文件)
 *   2. 创建 spdlog::logger 并注册所有 Sink
 *   3. 设置全局默认 logger
 *
 * @param config 配置结构体，包含所有初始化参数
 *
 * 设计决策:
 *   - 使用防重复初始化 guard (initialized_ 标志)
 *   - Sink 列表在 init 时一次性构建，运行时不再修改
 *   - 每个 Sink 有独立的日志级别和格式(pattern)
 */
void RoboLog::init(const RoboLogConfig &config) {
    if (initialized_) {
        return;  // 已初始化，避免重复创建
    }

    // 自动创建日志目录
    if (config.file_enabled || config.daily_rotating_enabled) {
        auto dir = std::filesystem::path(config.file_path).parent_path();
        if (!dir.empty()) std::filesystem::create_directories(dir);
    }
    if (config.error_file_enabled) {
        auto dir = std::filesystem::path(config.error_file_path).parent_path();
        if (!dir.empty()) std::filesystem::create_directories(dir);
    }

    std::vector<spdlog::sink_ptr> sinks;

    /**
     * Sink 1: 控制台输出
     *
     * 通常只输出 warn 及以上级别，避免高频 info 污染终端
     * 格式: [HH:MM:SS] [level] message
     * %^...%$: spdlog 的颜色标记，在终端中显示彩色输出
     */
    if (config.console_enabled) {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::warn);
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] [%s:%#] %v");
        sinks.push_back(console_sink);
    }

    /**
     * Sink 2: 运行日志文件(全量记录)
     *
     * 两种模式(二选一):
     *   A. daily_rotating_enabled = true:
     *      使用 daily_rotating_file_sink: 按天+按大小组合轮转
     *      文件名含日期: robot_2025-01-15.log, robot_2025-01-15_1.log
     *   B. daily_rotating_enabled = false:
     *      使用 rotating_file_sink: 仅按大小轮转
     *      文件名: robot.log, robot.log.1, robot.log.2
     *
     * 两种模式均为 trace 级别: 记录所有日志(包括高频实时数据)
     * 格式: [YYYY-MM-DD HH:MM:SS.mmm] [level    ] [thread_id] message
     *
     * pattern 说明:
     *   %Y-%m-%d %H:%M:%S.%e  → 完整时间戳(含毫秒)
     *   %^%-8l%$               → 左对齐 8 字符宽的带颜色级别名
     *   %t                     → 线程 ID
     *   %v                     → 日志消息
     */
    if (config.daily_rotating_enabled) {
        // 模式 A: 每日轮转 + 大小轮转(文件名含日期)
        auto daily_sink = std::make_shared<spdlog::sinks::daily_rotating_file_sink_mt>(
            config.file_path, config.daily_max_file_size, config.daily_max_files,
            config.rotation_hour, config.rotation_minute);
        daily_sink->set_level(spdlog::level::trace);
        daily_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] [%s:%#] %v");
        sinks.push_back(daily_sink);
    } else if (config.file_enabled) {
        // 模式 B: 仅按大小轮转
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            config.file_path, config.max_file_size, config.max_files);
        file_sink->set_level(spdlog::level::trace);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] [%s:%#] %v");
        sinks.push_back(file_sink);
    }

    /**
     * Sink 3: 错误日志文件(仅 error 及以上)
     *
     * 使用 daily_rotating_file_sink: 每日轮转 + 大小轮转
     * 只记录 error 和 critical 级别: 便于快速定位严重问题
     * 单独文件存储: 避免在大量 info 日志中淹没错误信息
     *
     * 文件命名示例:
     *   logs/robot_error_2025-01-15.log      ← 当天主文件
     *   logs/robot_error_2025-01-15_1.log    ← 当天第1次大小轮转
     *   logs/robot_error_2025-01-16.log      ← 新的一天
     */
    if (config.error_file_enabled) {
        auto error_sink = std::make_shared<spdlog::sinks::daily_rotating_file_sink_mt>(
            config.error_file_path, config.error_max_file_size, config.error_max_files,
            config.rotation_hour, config.rotation_minute);
        error_sink->set_level(spdlog::level::err);
        error_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] [%s:%#] %v");
        sinks.push_back(error_sink);
    }

    /**
     * 创建 Logger
     *
     * 两种模式:
     *   - 同步模式: std::make_shared<spdlog::logger>(name, sinks)
     *     直接在调用线程执行 I/O，延迟低但可能阻塞
     *
     *   - 异步模式: 先初始化线程池，再创建 logger
     *     消息入队后立即返回，I/O 由后台线程完成
     *     适合 1kHz 控制循环等实时性要求高的场景
     *
     * 注意: spdlog 的线程池是全局的，所有异步 logger 共享
     */
    if (config.async_enabled) {
        spdlog::init_thread_pool(config.async_queue_size, config.async_thread_count);
        logger_ = std::make_shared<spdlog::logger>("robo_log", sinks.begin(), sinks.end());
    } else {
        logger_ = std::make_shared<spdlog::logger>("robo_log", sinks.begin(), sinks.end());
    }

    // 设置 logger 的全局过滤级别
    // 低于此级别的日志将被忽略(不传递给任何 sink)
    logger_->set_level(config.level);

    // 设置为全局默认 logger
    // 之后可以通过 spdlog::info() 等全局函数访问
    spdlog::set_default_logger(logger_);

    initialized_ = true;
}

/**
 * 关闭 RoboLog
 *
 * 关闭顺序:
 *   1. flush(): 将缓冲区中的日志立即写入目标
 *   2. spdlog::shutdown(): 关闭全局线程池，等待所有异步任务完成
 *   3. logger_.reset(): 释放 logger 对象
 *   4. 重置 initialized_ 标志
 *
 * 注意: shutdown 后可以再次调用 init() 重新初始化
 */
void RoboLog::shutdown() {
    if (logger_) {
        logger_->flush();
    }
    spdlog::shutdown();
    logger_.reset();
    initialized_ = false;
}

/**
 * 设置日志级别(运行时动态调整)
 *
 * 应用场景:
 *   - 调试时切换到 debug/trace 级别
 *   - 生产环境切换到 info/warn 级别
 *   - 性能测试时关闭所有日志(off 级别)
 */
void RoboLog::set_level(spdlog::level::level_enum level) {
    if (logger_) {
        logger_->set_level(level);
    }
}

/**
 * 手动刷新日志缓冲区
 *
 * 何时需要手动 flush:
 *   - 程序即将退出前
 *   - 关键操作完成后(如错误恢复)
 *   - 长时间运行前确保日志已写入
 *
 * 注意: 高频调用 flush 会影响性能
 *       正常情况下 spdlog 会自动刷新(析构时)
 */
void RoboLog::flush() {
    if (logger_) {
        logger_->flush();
    }
}

// ============================================================
// 日志级别函数实现
// ============================================================
// 每个函数:
//   1. 检查 logger 是否已初始化(防 nullptr 崩溃)
//   2. 转发到 spdlog logger 的对应方法
//
// 使用 std::string 参数而非 spdlog 的 fmt 模板:
//   - 优点: 接口简单，编译速度快
//   - 缺点: 不支持 fmt::format 的编译期格式检查
//   - trade-off: 机器人场景通常日志格式固定，编译期检查收益不大

void RoboLog::trace(const std::string &msg, spdlog::source_loc loc) {
    if (logger_) logger_->log(loc, spdlog::level::trace, msg);
}

void RoboLog::debug(const std::string &msg, spdlog::source_loc loc) {
    if (logger_) logger_->log(loc, spdlog::level::debug, msg);
}

void RoboLog::info(const std::string &msg, spdlog::source_loc loc) {
    if (logger_) logger_->log(loc, spdlog::level::info, msg);
}

void RoboLog::warn(const std::string &msg, spdlog::source_loc loc) {
    if (logger_) logger_->log(loc, spdlog::level::warn, msg);
}

void RoboLog::error(const std::string &msg, spdlog::source_loc loc) {
    if (logger_) logger_->log(loc, spdlog::level::err, msg);
}

void RoboLog::critical(const std::string &msg, spdlog::source_loc loc) {
    if (logger_) logger_->log(loc, spdlog::level::critical, msg);
}
