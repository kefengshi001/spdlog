/**
 * @file robo_log.h
 * @brief RoboLog — 机械臂控制系统专用日志接口头文件
 *
 * 本文件定义了 RoboLog 日志系统的核心接口，是对 spdlog 的二次封装。
 *
 * 设计动机:
 *   spdlog 功能强大但配置复杂(需要手动创建 sink、设置 pattern、管理 logger 生命周期)。
 *   RoboLog 将这些细节封装为一个静态类，用户只需 init() → info/warn/error() → shutdown()。
 *
 * 架构概览:
 *   ┌─────────────────────────────────────────────────────┐
 *   │                   用户代码                           │
 *   │   RoboLog::init(config)                             │
 *   │   RoboLog::info("msg")  /  RoboLog::info("{}", x)  │
 *   │   RoboLog::shutdown()                               │
 *   └──────────────────────┬──────────────────────────────┘
 *                          │ 转发
 *   ┌──────────────────────▼──────────────────────────────┐
 *   │              RoboLog (静态类，本文件定义)              │
 *   │   持有 std::shared_ptr<spdlog::logger> logger_      │
 *   │   管理初始化状态 initialized_                        │
 *   └──────────────────────┬──────────────────────────────┘
 *                          │ 委托
 *   ┌──────────────────────▼──────────────────────────────┐
 *   │              spdlog::logger                          │
 *   │   将日志消息分发给所有注册的 sink                      │
 *   └──────────────────────┬──────────────────────────────┘
 *                          │ 分发
 *   ┌──────────┬───────────┼───────────┐
 *   ▼          ▼           ▼           ▼
 * console    file      error_file   (可扩展)
 *  sink       sink       sink
 *
 * 日志级别(从低到高):
 *   trace → debug → info → warn → error → critical → off
 *
 * 两种接口风格:
 *   1. 简单字符串:  RoboLog::info("系统启动")          — 接口简单，编译快
 *   2. 格式化模板:  RoboLog::info("angle={:.2f}", 45.0) — 支持 spdlog 的 fmt 格式化
 *
 * 线程安全:
 *   - 所有公共方法均为 static，可从任意线程调用
 *   - 底层使用 _mt(multi-thread) 后缀的 sink，内部已有互斥锁保护
 *   - 异步模式下，日志消息入队后立即返回，I/O 由后台线程池完成
 *
 * 使用示例:
 *   @code
 *   // 1. 初始化(通常在 main 开头)
 *   RoboLogConfig config;
 *   config.level = spdlog::level::debug;
 *   RoboLog::init(config);
 *
 *   // 2. 记录日志(任意位置)
 *   RoboLog::info("系统启动完成");
 *   RoboLog::warn("关节接近限位: angle={:.1f}", 89.5);
 *   RoboLog::error("编码器异常: Joint {}", 3);
 *
 *   // 3. 关闭(通常在 main 末尾)
 *   RoboLog::shutdown();
 *   @endcode
 */

#pragma once

// ============================================================
// spdlog 核心头文件
// ============================================================

#include "spdlog/spdlog.h"
// spdlog 主头文件，提供:
//   - spdlog::logger 类(日志器核心)
//   - spdlog::level::level_enum (日志级别枚举: trace/debug/info/warn/error/critical/off)
//   - spdlog::set_default_logger() 等全局函数
//   - fmt 格式化支持(底层使用 fmtlib)

#include "spdlog/sinks/stdout_color_sinks.h"
// 控制台彩色输出 sink:
//   - stdout_color_sink_mt: 线程安全版本(_mt = multi-thread)
//   - 支持 ANSI 颜色转义码，在终端中显示彩色日志
//   - 使用 %^...%$ 标记包裹的部分会显示颜色(不同级别不同颜色)

#include "spdlog/sinks/basic_file_sink.h"
// 基础文件 sink:
//   - basic_file_sink_mt: 线程安全版本
//   - 简单地将日志追加到文件末尾，不做轮转
//   - 适用于错误日志等低频、需要永久保留的场景

#include "spdlog/sinks/rotating_file_sink.h"
// 轮转文件 sink:
//   - rotating_file_sink_mt: 线程安全版本
//   - 按文件大小轮转: 当文件达到 max_size 时，创建新文件
//   - 保留最近 N 个文件(如 robot.log, robot.log.1, robot.log.2, ...)
//   - 适用于运行日志等高频、需要限制磁盘空间的场景

#include "spdlog/async.h"
// 异步日志支持:
//   - spdlog::init_thread_pool(): 初始化全局异步线程池
//   - spdlog::async_logger: 异步日志器，消息入队后立即返回
//   - 适用场景: 高频控制循环(1kHz)中不能因日志 I/O 阻塞主循环
//   - 注意: spdlog 的线程池是全局单例，所有异步 logger 共享

#include "daily_rotating_file_sink.h"
// 自定义 Sink: 每日轮转 + 大小轮转
//   - 按天轮转: 跨日时自动创建新文件(文件名含日期: robot_2025-01-15.log)
//   - 按大小轮转: 同一天内文件过大时自动切分(文件名含序号: robot_2025-01-15_1.log)
//   - 组合了 daily_file_sink 和 rotating_file_sink 的功能

// ============================================================
// C++ 标准库
// ============================================================

#include <memory>
// std::shared_ptr: 用于管理 spdlog::logger 的生命周期
// 选择 shared_ptr 而非 unique_ptr 的原因:
//   - spdlog 内部会持有 logger 的弱引用(weak_ptr)
//   - set_default_logger() 会增加引用计数
//   - 确保 logger 在所有引用释放后才被销毁

#include <string>
// std::string: 用于日志消息参数和文件路径配置

// ============================================================
// 日志配置结构体
// ============================================================

/**
 * @struct RoboLogConfig
 * @brief RoboLog 初始化配置参数
 *
 * 使用结构体而非多个参数的原因:
 *   1. 参数多(11 个)，用结构体更清晰
 *   2. 提供合理默认值，用户只需修改关心的字段
 *   3. 便于预定义多套配置(见 robo_log_config.h)
 *
 * 使用示例:
 *   @code
 *   // 使用全部默认值
 *   RoboLog::init(RoboLogConfig());
 *
 *   // 自定义部分配置
 *   RoboLogConfig config;
 *   config.level = spdlog::level::debug;
 *   config.async_enabled = false;
 *   RoboLog::init(config);
 *   @endcode
 */
struct RoboLogConfig {
    /**
     * @brief 全局日志过滤级别
     *
     * 低于此级别的日志将被忽略(不传递给任何 sink)。
     * 这是第一道过滤，比各 sink 自身的级别过滤更高效。
     *
     * 级别从低到高:
     *   trace(0) → debug(1) → info(2) → warn(3) → error(4) → critical(5) → off(6)
     *
     * 典型设置:
     *   - 开发调试: debug 或 trace
     *   - 生产环境: info 或 warn
     *   - 性能测试: off(完全关闭日志)
     */
    spdlog::level::level_enum level = spdlog::level::info;

    /**
     * @brief 是否启用控制台输出
     *
     * 启用后，日志将输出到 stdout(带 ANSI 颜色)。
     * 生产环境通常关闭以减少 I/O 开销。
     */
    bool console_enabled = true;

    /**
     * @brief 是否启用文件日志
     *
     * 启用后，日志将写入 file_path 指定的文件。
     * 使用 rotating_file_sink，按大小自动轮转。
     */
    bool file_enabled = true;

    /**
     * @brief 运行日志文件路径
     *
     * 默认写入 logs/robot.log。
     * 轮转后的旧文件命名为: robot.log.1, robot.log.2, ...
     * 目录需提前创建，spdlog 不会自动创建目录。
     */
    std::string file_path = "logs/robot.log";

    /**
     * @brief 单个日志文件最大大小(字节)
     *
     * 当文件达到此大小时，触发轮转(rename 为 .1，创建新文件)。
     * 默认 10MB，适合大多数场景。
     * 高频日志场景可增大至 50MB+。
     */
    size_t max_file_size = 50 * 1024 * 1024;  // 50MB

    /**
     * @brief 最大保留文件数
     *
     * 轮转时保留的旧文件数量。
     * 例如 max_files=10: robot.log, .1, .2, .3, .4, .5, .6, .7, .8, .9(最旧的 .10 被删除)
     * 总磁盘占用 ≈ max_file_size × max_files
     */
    size_t max_files = 10;

    /**
     * @brief 是否启用错误日志单独文件
     *
     * 启用后，error 和 critical 级别的日志会额外写入独立文件。
     * 便于快速定位严重问题，无需在大量 info 日志中搜索。
     */
    bool error_file_enabled = true;

    /**
     * @brief 错误日志文件路径
     *
     * 仅记录 error 及以上级别的日志。
     * 使用 daily_rotating_file_sink，支持每日轮转 + 大小轮转。
     * 文件命名: robot_error_2025-01-15.log, robot_error_2025-01-15_1.log
     */
    std::string error_file_path = "logs/robot_error.log";

    /**
     * @brief 错误日志单文件最大大小(字节)
     *
     * 同一天内，当错误日志文件达到此大小时触发大小轮转。
     * 默认 50MB。
     */
    size_t error_max_file_size = 50 * 1024 * 1024;  // 50MB

    /**
     * @brief 错误日志同一天内最大保留文件数(含当前文件)
     *
     * 例如 error_max_files=10:
     *   当天主文件(.log) + 轮转文件(_1.log ~ _9.log)
     *   总计 10 个文件，最旧的会被删除
     */
    size_t error_max_files = 10;

    /**
     * @brief 是否启用异步模式
     *
     * 异步模式下，日志消息先入队，由后台线程执行实际 I/O。
     * 优点: 不阻塞调用线程(适合 1kHz 控制循环)
     * 缺点: 程序崩溃时可能丢失队列中未写入的日志
     *
     * 同步模式(async_enabled=false):
     *   - 日志在调用线程直接写入，延迟可预测
     *   - 适合调试阶段(确保日志完整)
     *
     * 异步模式(async_enabled=true):
     *   - 日志入队后立即返回，I/O 由线程池完成
     *   - 适合生产环境(避免 I/O 阻塞控制循环)
     */
    bool async_enabled = true;

    /**
     * @brief 异步队列大小
     *
     * 异步模式下，内部环形队列的容量。
     * 队列满时，新的日志消息将被丢弃(默认阻塞等待)。
     * 默认 8192 条，高频场景可增大至 65536+。
     */
    size_t async_queue_size = 65536;

    /**
     * @brief 异步线程池线程数
     *
     * 处理异步日志的后台线程数量。
     * 默认 1 个线程，通常足够(日志 I/O 是串行写入)。
     * 极高吞吐场景可增加到 2。
     */
    size_t async_thread_count = 2;

    // ============================================================
    // 每日轮转配置(daily_rotating_file_sink)
    // ============================================================
    // 当 daily_rotating_enabled = true 时，使用 daily_rotating_file_sink
    // 替代普通的 rotating_file_sink，提供按天+按大小的组合轮转策略。
    // 此时 file_enabled 的设置会被忽略。

    /**
     * @brief 是否启用每日轮转
     *
     * 启用后，使用 daily_rotating_file_sink 替代 rotating_file_sink:
     *   - 按天轮转: 跨日时自动创建新文件(文件名含日期)
     *   - 按大小轮转: 同一天内文件过大时自动切分(文件名含序号)
     *
     * 文件命名示例(假设 file_path = "logs/robot.log"):
     *   logs/robot_2025-01-15.log      ← 当天主文件
     *   logs/robot_2025-01-15_1.log    ← 当天第1次大小轮转
     *   logs/robot_2025-01-15_2.log    ← 当天第2次大小轮转
     *   logs/robot_2025-01-16.log      ← 新的一天
     *
     * @note 优先级高于 file_enabled，两者同时启用时以此为准
     */
    bool daily_rotating_enabled = false;

    /**
     * @brief 每日轮转的单文件最大大小(字节)
     *
     * 同一天内，当日志文件达到此大小时触发大小轮转。
     * 默认 50MB，适合大多数场景。
     *
     * 总磁盘占用 ≈ daily_max_file_size × daily_max_files
     */
    size_t daily_max_file_size = 50 * 1024 * 1024;  // 50MB

    /**
     * @brief 同一天内最大保留文件数(含当前文件)
     *
     * 例如 daily_max_files=10:
     *   当天主文件(.log) + 轮转文件(_1.log ~ _9.log)
     *   总计 10 个文件，最旧的会被删除
     */
    size_t daily_max_files = 10;

    /**
     * @brief 每日轮转时刻-小时(0-23)
     *
     * 默认 0 表示午夜轮转。
     * 可改为其他值，如 6 表示每天早上 6 点轮转。
     */
    int rotation_hour = 0;

    /**
     * @brief 每日轮转时刻-分钟(0-59)
     *
     * 配合 rotation_hour 使用。
     * 例如 rotation_hour=6, rotation_minute=30 表示每天 06:30 轮转。
     */
    int rotation_minute = 0;
};

// ============================================================
// RoboLog 核心类
// ============================================================

/**
 * @class RoboLog
 * @brief 机械臂控制系统日志接口(静态类)
 *
 * 设计为纯静态类(所有成员均为 static)，原因:
 *   1. 日志系统通常是全局单例，无需多个实例
 *   2. 静态接口更简洁: RoboLog::info("msg") vs robo_log.info("msg")
 *   3. 无需传递 logger 实例，任意位置可直接调用
 *
 * 生命周期:
 *   程序启动 → init(config) → [使用日志] → shutdown() → 程序退出
 *
 * 初始化状态:
 *   - init() 后 initialized_ = true，所有日志方法正常工作
 *   - shutdown() 后 initialized_ = false，日志方法静默忽略
 *   - 重复调用 init() 会被忽略(防重复初始化)
 *
 * @note 所有公共方法均为线程安全的 static 方法
 */
class RoboLog {
public:
    /**
     * @brief 初始化日志系统
     *
     * 根据配置创建 sink 组合和 logger 实例。
     * 通常在 main() 开头调用一次。
     *
     * 内部流程:
     *   1. 检查是否已初始化(防重复)
     *   2. 根据配置创建 sink 列表(console + file + error_file)
     *   3. 创建 spdlog::logger，注册所有 sink
     *   4. 设置全局默认 logger(spdlog::set_default_logger)
     *
     * @param config 配置参数，使用默认值可不传
     *
     * @note 重复调用会被静默忽略(不会创建新的 logger)
     * @see RoboLogConfig 配置参数说明
     */
    static void init(const RoboLogConfig &config = RoboLogConfig());

    /**
     * @brief 关闭日志系统
     *
     * 刷新缓冲区并释放所有资源。
     * 通常在 main() 末尾、return 之前调用。
     *
     * 内部流程:
     *   1. flush(): 将缓冲区中的日志立即写入目标
     *   2. spdlog::shutdown(): 关闭全局线程池，等待异步任务完成
     *   3. logger_.reset(): 释放 logger 对象
     *   4. 重置 initialized_ 标志
     *
     * @note shutdown() 后可再次调用 init() 重新初始化
     * @note 不调用 shutdown() 时，spdlog 会在全局析构时自动清理
     */
    static void shutdown();

    /**
     * @brief 运行时动态调整日志级别
     *
     * 允许在不重启程序的情况下改变日志过滤级别。
     *
     * 典型场景:
     *   - 调试时切换到 debug/trace 级别获取详细信息
     *   - 生产环境切换到 info/warn 减少日志量
     *   - 性能测试时设置为 off 完全关闭日志
     *
     * @param level 新的日志过滤级别
     *
     * @code
     * RoboLog::set_level(spdlog::level::debug);  // 临时开启 debug
     * RoboLog::debug("这条现在会输出");
     * RoboLog::set_level(spdlog::level::info);   // 恢复
     * @endcode
     */
    static void set_level(spdlog::level::level_enum level);

    /**
     * @brief 手动刷新日志缓冲区
     *
     * 将缓冲区中尚未写入的日志立即刷出。
     *
     * 何时需要手动调用:
     *   - 程序即将退出前(确保最后几条日志不丢失)
     *   - 关键操作完成后(如错误恢复成功)
     *   - 长时间无日志输出前(确保之前的日志已落盘)
     *
     * @note 高频调用 flush() 会影响性能，正常情况下无需调用
     * @note shutdown() 内部会自动调用 flush()
     */
    static void flush();

    /** @brief 获取底层 spdlog logger (供宏使用) */
    static std::shared_ptr<spdlog::logger> get_logger() { return logger_; }

    // ============================================================
    // 基本日志接口(接受 std::string)
    // ============================================================
    //
    // 这组接口接受 std::string 参数，不支持格式化占位符。
    // 适用于日志消息已经是完整字符串的场景。
    //
    // 使用示例:
    //   RoboLog::info("系统启动完成");
    //   RoboLog::error("通信中断");
    //
    // 与格式化接口的区别:
    //   - 基本接口: RoboLog::info("固定消息")           — 简单直接
    //   - 格式化接口: RoboLog::info("val={}", 42)       — 支持占位符
    //   - 两者可混用，根据是否需要格式化选择

    /** @brief 记录 trace 级别日志(高频实时数据，生产环境通常关闭) */
    static void trace(const std::string &msg, spdlog::source_loc loc);

    /** @brief 记录 debug 级别日志(调试信息，生产环境通常关闭) */
    static void debug(const std::string &msg, spdlog::source_loc loc);

    /** @brief 记录 info 级别日志(正常运行信息，如启动/停止/模式切换) */
    static void info(const std::string &msg, spdlog::source_loc loc);

    /** @brief 记录 warn 级别日志(警告，如接近限位、通信延迟) */
    static void warn(const std::string &msg, spdlog::source_loc loc);

    /** @brief 记录 error 级别日志(错误，如编码器异常、通信中断) */
    static void error(const std::string &msg, spdlog::source_loc loc);

    /** @brief 记录 critical 级别日志(严重错误，如碰撞检测、紧急停机) */
    static void critical(const std::string &msg, spdlog::source_loc loc);

    // ============================================================
    // 格式化日志接口(支持 spdlog 的 fmt 格式化)
    // ============================================================
    //
    // 这组模板接口支持 spdlog 的格式化语法(基于 fmtlib)。
    // 使用 fmt::format_string 作为第一个参数，后续为可变参数。
    //
    // 格式化语法示例:
    //   {}          → 默认格式      RoboLog::info("val={}", 42)        → "val=42"
    //   {:.2f}      → 浮点 2 位小数  RoboLog::info("pi={:.2f}", 3.14)  → "pi=3.14"
    //   {:>10}      → 右对齐 10 字符  RoboLog::info("{:>10}", "hi")    → "        hi"
    //   {:08x}      → 十六进制补零    RoboLog::info("addr={:08x}", 255)→ "addr=000000ff"
    //
    // 编译期安全:
    //   fmt::format_string 会在编译期检查格式字符串与参数类型的匹配。
    //   如果格式串和参数不匹配，编译时报错而非运行时崩溃。
    //
    // 完美转发:
    //   Args&&...args 使用万能引用(perfect forwarding)，
    //   避免不必要的拷贝，支持移动语义。

    /**
     * @brief 格式化输出 trace 级别日志
     * @tparam Args 可变模板参数类型
     * @param fmt 格式字符串(编译期检查)
     * @param args 格式化参数
     */
    template<typename... Args>
    static void trace(fmt::format_string<Args...> fmt, Args &&...args) {
        if (logger_) logger_->trace(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief 格式化输出 debug 级别日志
     * @tparam Args 可变模板参数类型
     * @param fmt 格式字符串(编译期检查)
     * @param args 格式化参数
     */
    template<typename... Args>
    static void debug(fmt::format_string<Args...> fmt, Args &&...args) {
        if (logger_) logger_->debug(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief 格式化输出 info 级别日志
     * @tparam Args 可变模板参数类型
     * @param fmt 格式字符串(编译期检查)
     * @param args 格式化参数
     */
    template<typename... Args>
    static void info(fmt::format_string<Args...> fmt, Args &&...args) {
        if (logger_) logger_->info(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief 格式化输出 warn 级别日志
     * @tparam Args 可变模板参数类型
     * @param fmt 格式字符串(编译期检查)
     * @param args 格式化参数
     */
    template<typename... Args>
    static void warn(fmt::format_string<Args...> fmt, Args &&...args) {
        if (logger_) logger_->warn(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief 格式化输出 error 级别日志
     * @tparam Args 可变模板参数类型
     * @param fmt 格式字符串(编译期检查)
     * @param args 格式化参数
     */
    template<typename... Args>
    static void error(fmt::format_string<Args...> fmt, Args &&...args) {
        if (logger_) logger_->error(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief 格式化输出 critical 级别日志
     * @tparam Args 可变模板参数类型
     * @param fmt 格式字符串(编译期检查)
     * @param args 格式化参数
     */
    template<typename... Args>
    static void critical(fmt::format_string<Args...> fmt, Args &&...args) {
        if (logger_) logger_->critical(fmt, std::forward<Args>(args)...);
    }

private:
    /**
     * @brief 底层 spdlog 日志器实例
     *
     * 所有日志方法最终都委托给这个 logger。
     * 使用 shared_ptr 管理生命周期:
     *   - init() 时创建
     *   - 同时被 set_default_logger() 引用(增加引用计数)
     *   - shutdown() 时 reset() 释放
     *
     * @note 只有 init() 成功后才非 nullptr
     * @note 所有日志方法内部都会检查 nullptr(防崩溃)
     */
    static std::shared_ptr<spdlog::logger> logger_;

    /**
     * @brief 初始化状态标志
     *
     * 防止重复初始化:
     *   - init() 成功后设为 true
     *   - shutdown() 后重置为 false
     *   - init() 检查此标志，已初始化则直接返回
     */
    static bool initialized_;
};

// ============================================================
// 日志宏(自动捕获文件名和行号)
// ============================================================
//
// 为什么用宏而不是函数:
//   __FILE__、__LINE__、__func__ 是预处理器宏，必须在调用处展开
//   才能获取调用者的真实位置。函数参数中的默认值无法可靠捕获
//   调用者位置(特别是与可变参数模板组合时)。
//
//   spdlog 自身也是用宏实现源位置捕获: SPDLOG_INFO, SPDLOG_ERROR 等。
//
// 两类宏:
//   ROBOLOG_xxx(msg)        → 简单字符串，如 ROBOLOG_INFO("系统启动")
//   ROBOLOG_xxx_FMT(fmt,…) → 格式化输出，如 ROBOLOG_INFO_FMT("val={}", 42)

#define ROBOLOG_TRACE(msg) \
    RoboLog::trace(msg, spdlog::source_loc{__FILE__, __LINE__, __func__})
#define ROBOLOG_DEBUG(msg) \
    RoboLog::debug(msg, spdlog::source_loc{__FILE__, __LINE__, __func__})
#define ROBOLOG_INFO(msg) \
    RoboLog::info(msg, spdlog::source_loc{__FILE__, __LINE__, __func__})
#define ROBOLOG_WARN(msg) \
    RoboLog::warn(msg, spdlog::source_loc{__FILE__, __LINE__, __func__})
#define ROBOLOG_ERROR(msg) \
    RoboLog::error(msg, spdlog::source_loc{__FILE__, __LINE__, __func__})
#define ROBOLOG_CRITICAL(msg) \
    RoboLog::critical(msg, spdlog::source_loc{__FILE__, __LINE__, __func__})

#define ROBOLOG_TRACE_FMT(fmt, ...) \
    do { if (RoboLog::get_logger()) RoboLog::get_logger()->log( \
        spdlog::source_loc{__FILE__, __LINE__, __func__}, spdlog::level::trace, fmt, ##__VA_ARGS__); } while(0)
#define ROBOLOG_DEBUG_FMT(fmt, ...) \
    do { if (RoboLog::get_logger()) RoboLog::get_logger()->log( \
        spdlog::source_loc{__FILE__, __LINE__, __func__}, spdlog::level::debug, fmt, ##__VA_ARGS__); } while(0)
#define ROBOLOG_INFO_FMT(fmt, ...) \
    do { if (RoboLog::get_logger()) RoboLog::get_logger()->log( \
        spdlog::source_loc{__FILE__, __LINE__, __func__}, spdlog::level::info, fmt, ##__VA_ARGS__); } while(0)
#define ROBOLOG_WARN_FMT(fmt, ...) \
    do { if (RoboLog::get_logger()) RoboLog::get_logger()->log( \
        spdlog::source_loc{__FILE__, __LINE__, __func__}, spdlog::level::warn, fmt, ##__VA_ARGS__); } while(0)
#define ROBOLOG_ERROR_FMT(fmt, ...) \
    do { if (RoboLog::get_logger()) RoboLog::get_logger()->log( \
        spdlog::source_loc{__FILE__, __LINE__, __func__}, spdlog::level::err, fmt, ##__VA_ARGS__); } while(0)
#define ROBOLOG_CRITICAL_FMT(fmt, ...) \
    do { if (RoboLog::get_logger()) RoboLog::get_logger()->log( \
        spdlog::source_loc{__FILE__, __LINE__, __func__}, spdlog::level::critical, fmt, ##__VA_ARGS__); } while(0)
