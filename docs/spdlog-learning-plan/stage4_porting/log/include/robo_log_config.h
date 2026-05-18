/**
 * @file robo_log_config.h
 * @brief RoboLog 预设配置模板
 *
 * 本文件提供四套常用的 RoboLogConfig 预设，覆盖典型使用场景:
 *
 *   配置名称          适用场景              关键特点
 *   ─────────────────────────────────────────────────────────
 *   debug_config()     开发调试阶段          同步模式 + 控制台输出 + debug 级别
 *   production_config() 生产部署             异步模式 + 无控制台 + info 级别
 *   high_performance_config() 高频数据采集   异步模式 + 大队列 + trace 级别
 *   daily_config()     需要日期归档的场景    每日轮转 + 大小轮转 + 日期文件名
 *
 * 使用方式:
 *   @code
 *   // 直接使用预设
 *   RoboLog::init(debug_config());
 *
 *   // 基于预设微调
 *   auto config = production_config();
 *   config.console_enabled = true;  // 临时开启控制台
 *   RoboLog::init(config);
 *   @endcode
 *
 * 设计说明:
 *   所有函数声明为 inline，原因:
 *   1. 函数定义在头文件中(非类成员)，multiple inclusion 会导致链接重复定义错误
 *   2. inline 告诉链接器: 多个翻译单元中的同名定义是同一个函数，允许合并
 *   3. 这些函数体较小，inline 也有利于编译器内联优化(虽然此处不是主要目的)
 */

#pragma once

#include "robo_log.h"
// 引入 RoboLogConfig 结构体定义和 spdlog::level 枚举

// ============================================================
// 调试模式配置
// ============================================================

/**
 * @brief 调试模式配置 — 开发阶段使用
 *
 * 特点:
 *   - 同步模式(async_enabled = false):
 *     日志在调用线程直接写入，延迟可预测
 *     程序崩溃时不会丢失队列中的日志(异步模式可能丢失)
 *     便于用 gdb 调试时观察实时日志输出
 *
 *   - 控制台输出(console_enabled = true):
 *     日志同时输出到终端，开发时无需反复查看文件
 *     控制台只显示 warn 及以上(由 RoboLog::init 中的 sink 级别控制)
 *
 *   - debug 级别:
 *     包含 debug 和 info 信息，便于跟踪程序执行流程
 *     不含 trace(避免过多高频数据干扰调试)
 *
 *   - 全量文件记录(file_enabled = true):
 *     所有 debug 及以上日志写入文件，事后可回溯分析
 *
 * 适用场景:
 *   - 日常开发、单元测试、功能验证
 *   - gdb/lldb 调试时跟踪程序状态
 *   - 排查 bug 时需要完整日志
 *
 * @return 配置好的 RoboLogConfig 实例
 */
inline RoboLogConfig debug_config() {
    RoboLogConfig config;
    config.level = spdlog::level::debug;        // 记录 debug 及以上
    config.console_enabled = true;              // 开启控制台输出
    config.file_enabled = true;                 // 开启文件记录
    config.error_file_enabled = true;           // 开启错误日志单独文件
    config.async_enabled = false;               // 同步模式，便于调试
    return config;
}

// ============================================================
// 生产模式配置
// ============================================================

/**
 * @brief 生产模式配置 — 部署上线使用
 *
 * 特点:
 *   - 异步模式(async_enabled = true):
 *     日志入队后立即返回，不阻塞主业务逻辑
 *     适合机器人控制等实时性要求高的场景
 *     代价: 程序异常退出时可能丢失队列中最后几条日志
 *
 *   - 无控制台输出(console_enabled = false):
 *     生产环境通常无终端，或通过 systemd/journal 查看
 *     减少 stdout I/O 开销
 *
 *   - info 级别:
 *     只记录关键运行信息，不记录 debug 细节
 *     平衡日志量和可观测性
 *
 *   - 适中的异步队列(queue_size = 16384):
 *     比默认值(8192)大一倍，应对突发日志峰值
 *     1 个后台线程通常足够(文件写入是串行的)
 *
 * 适用场景:
 *   - 正式部署的机器人系统
 *   - 无人值守的自动化设备
 *   - 对性能敏感的实时控制场景
 *
 * @return 配置好的 RoboLogConfig 实例
 */
inline RoboLogConfig production_config() {
    RoboLogConfig config;
    config.level = spdlog::level::info;         // 只记录 info 及以上
    config.console_enabled = false;             // 生产环境不输出到控制台
    config.file_enabled = true;                 // 开启文件记录
    config.error_file_enabled = true;           // 开启错误日志单独文件
    config.async_enabled = true;                // 异步模式，不阻塞主循环
    config.async_queue_size = 16384;            // 较大队列，应对突发日志
    config.async_thread_count = 1;              // 单后台线程(文件写入串行)
    return config;
}

// ============================================================
// 高性能模式配置
// ============================================================

/**
 * @brief 高性能模式配置 — 高频数据记录场景
 *
 * 特点:
 *   - trace 级别:
 *     记录所有日志，包括高频实时数据(关节角度、速度、力矩等)
 *     用于数据采集、性能分析、运动轨迹回放
 *     ⚠️ 日志量极大，仅在需要时开启
 *
 *   - 大文件轮转(max_file_size = 50MB, max_files = 10):
 *     总磁盘占用 ≈ 500MB(50MB × 10 个文件)
 *     适合长时间运行的数据采集任务
 *
 *   - 大异步队列(queue_size = 65536):
 *     1kHz 控制循环 × 6 轴 = 6000 条/秒
 *     65536 队列可缓冲约 11 秒的峰值数据
 *     避免因队列满导致日志丢失
 *
 *   - 双后台线程(thread_count = 2):
 *     两个线程并行处理队列中的日志消息
 *     提高日志吞吐量，应对高并发写入
 *
 *   - 无控制台输出:
 *     控制台 I/O 太慢，会成为瓶颈
 *
 * 适用场景:
 *   - 运动控制系统的数据采集(关节角度、力矩曲线)
 *   - 性能基准测试(测量控制循环延迟)
 *   - 离线数据分析(需要完整的时序数据)
 *
 * @return 配置好的 RoboLogConfig 实例
 */
inline RoboLogConfig high_performance_config() {
    RoboLogConfig config;
    config.level = spdlog::level::trace;        // 记录所有级别(含高频 trace)
    config.console_enabled = false;             // 关闭控制台，减少 I/O
    config.file_enabled = true;                 // 开启文件记录
    config.max_file_size = 50 * 1024 * 1024;   // 50MB/文件
    config.max_files = 10;                      // 保留 10 个文件(总计 ~500MB)
    config.error_file_enabled = true;           // 开启错误日志单独文件
    config.async_enabled = true;                // 异步模式
    config.async_queue_size = 65536;            // 大队列，缓冲峰值数据
    config.async_thread_count = 2;              // 双线程，提高吞吐
    return config;
}

// ============================================================
// 每日轮转模式配置
// ============================================================

/**
 * @brief 每日轮转模式配置 — 需要按日期归档日志的场景
 *
 * 特点:
 *   - 每日轮转(daily_rotating_enabled = true):
 *     跨日时自动创建新文件，文件名含日期
 *     例: robot_2025-01-15.log, robot_2025-01-16.log
 *
 *   - 大小轮转(同一天内):
 *     单日日志过多时自动切分，文件名含序号
 *     例: robot_2025-01-15.log → robot_2025-01-15_1.log → robot_2025-01-15_2.log
 *
 *   - 单文件 50MB，同一天最多保留 10 个文件:
 *     总磁盘占用 ≈ 500MB/天
 *
 *   - debug 级别 + 同步模式:
 *     便于调试时观察日志，不丢失
 *
 *   - 控制台 + 文件双输出:
 *     控制台实时查看，文件按日期归档
 *
 * 适用场景:
 *   - 需要按日期查找历史日志(如"上周三的日志")
 *   - 长期运行的设备，日志需要定期归档清理
 *   - 合规要求保留每日运行记录
 *
 * @return 配置好的 RoboLogConfig 实例
 */
inline RoboLogConfig daily_config() {
    RoboLogConfig config;
    config.level = spdlog::level::debug;                // 记录 debug 及以上
    config.console_enabled = true;                      // 开启控制台输出
    config.file_enabled = false;                        // 不使用普通 rotating sink
    config.daily_rotating_enabled = true;               // 启用每日轮转
    config.daily_max_file_size = 50 * 1024 * 1024;     // 50MB/文件
    config.daily_max_files = 10;                        // 同一天最多 10 个文件
    config.rotation_hour = 0;                           // 每天 00:00 轮转
    config.rotation_minute = 0;
    config.error_file_enabled = true;                   // 开启错误日志单独文件
    config.async_enabled = false;                       // 同步模式，便于调试
    return config;
}
