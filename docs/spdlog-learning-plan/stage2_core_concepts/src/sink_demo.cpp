// spdlog 阶段 2: Sink 概念演示
// 演示各类内置 Sink 的使用
//
// Sink 是 spdlog 的输出目标组件，负责将格式化后的日志消息写入具体的目的地。
// spdlog 的架构设计为: Logger → Formatter(格式化) → Sink(输出)
//
// spdlog 内置了多种 Sink：
//   - 控制台 Sink: stdout/stderr，支持 ANSI 颜色
//   - 文件 Sink: basic_file（基础）、rotating_file（按大小轮转）、daily_file（按天轮转）
//   - 空 Sink: null_sink，丢弃所有日志（用于测试/禁用日志）
//   - 系统日志 Sink: syslog（Linux）、winlog（Windows）
//   - 网络 Sink: tcp_sink、udp_sink
//
// 每个 Sink 都可以独立设置日志级别，实现不同目标输出不同级别的日志

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/null_sink.h"

#include <iostream>
#include <memory>

// 演示控制台 Sink 的两种类型
// 控制台 Sink 是最常用的 Sink，用于实时查看日志输出
void console_sink_demo() {
    std::cout << "\n=== 1. 控制台 Sink ===" << std::endl;

    // stdout_color_mt: 输出到标准输出(stdout)，支持 ANSI 颜色转义序列
    // 不同日志级别会以不同颜色显示（如 info 绿色、warn 黄色、error 红色）
    // _mt 后缀表示线程安全（multi-threaded），适用于多线程环境
    auto console = spdlog::stdout_color_mt("console");
    console->info("stdout_color_sink: 带颜色的控制台输出");

    // stderr_color_mt: 输出到标准错误(stderr)，同样支持颜色
    // stderr 通常用于输出错误/警告信息，与 stdout 分离便于重定向和日志分离
    // 例如: ./app > output.log 时，stdout 写入文件，stderr 仍显示在终端
    auto stderr_logger = spdlog::stderr_color_mt("stderr");
    stderr_logger->info("stderr_color_sink: 标准错误输出");

    spdlog::drop("console");
    spdlog::drop("stderr");
}

// 演示三种文件 Sink 的使用
// 文件 Sink 将日志写入磁盘文件，是生产环境中最重要的持久化手段
// spdlog 提供三种文件策略，适用于不同的日志管理需求
void file_sink_demo() {
    std::cout << "\n=== 2. 文件 Sink ===" << std::endl;

    // 基础文件 Sink（basic_file_sink）
    // 参数 1: 日志文件路径
    // 参数 2: truncate=true 表示每次启动清空文件，false 表示追加模式
    // 特点: 简单直接，持续写入同一个文件，不会自动轮转
    // 适用: 开发调试、短期运行的程序
    auto basic = spdlog::basic_logger_mt("basic_file", "logs/basic.log", true);
    basic->info("basic_file_sink: 基础文件输出");

    // 轮转文件 Sink（rotating_file_sink）—— 按文件大小轮转
    // 参数 1: 基础文件名
    // 参数 2: 单个文件最大大小（字节），超过后创建新文件
    // 参数 3: 最大保留的备份文件数（旧文件会被删除）
    // 轮转后的文件名会自动添加编号后缀，如: rotating.log, rotating.log.1, rotating.log.2
    // 适用: 生产环境，防止单个日志文件无限增长撑满磁盘
    auto rotating = spdlog::rotating_logger_mt("rotating", "logs/rotating.log",
        1048576 * 5, 3);  // 5MB, 3 backups
    rotating->info("rotating_file_sink: 按大小轮转");

    // 每日文件 Sink（daily_file_sink）—— 按时间轮转
    // 参数 1: 基础文件名
    // 参数 2: 每天几点创建新文件（24 小时制）
    // 参数 3: 几分创建新文件
    // 每天在指定时刻自动创建新文件，文件名自动附加日期后缀（如 daily_2026-05-14.log）
    // 适用: 需要按日期归档日志的场景，便于按天检索和清理
    auto daily = spdlog::daily_logger_mt("daily", "logs/daily.log", 2, 30);
    daily->info("daily_file_sink: 每日轮转");

    spdlog::drop("basic_file");
    spdlog::drop("rotating");
    spdlog::drop("daily");
}

// 演示空 Sink（null_sink）
// null_sink 会丢弃所有接收到的日志消息，不做任何输出
// 用途：
//   1. 单元测试中禁用日志输出，避免测试输出被日志干扰
//   2. 运行时通过配置动态禁用某个 Logger 的输出
//   3. 性能测试中消除 I/O 开销，仅衡量日志框架本身的开销
void null_sink_demo() {
    std::cout << "\n=== 3. 空 Sink ===" << std::endl;

    // spdlog::create<>() 使用指定的 Sink 类型创建 Logger 并注册到全局 Registry
    // 模板参数 null_sink_mt 指定使用线程安全的空 Sink
    // 与工厂函数不同，create<>() 的 Sink 类型由模板参数决定，更灵活
    auto null_logger = spdlog::create<spdlog::sinks::null_sink_mt>("null");
    null_logger->info("This message is discarded");  // 这条日志会被 null_sink 直接丢弃
    std::cout << "null_sink: 消息被丢弃（无输出）" << std::endl;

    spdlog::drop("null");
}

// 演示多 Sink 组合 + 每个 Sink 独立的级别过滤
// 这是 spdlog 最实用的模式之一：同一条日志按级别分流到不同目标
// 典型生产场景: 控制台只看警告，文件记录全部，错误单独归档
void multi_sink_with_levels_demo() {
    std::cout << "\n=== 4. 多 Sink + 级别过滤 ===" << std::endl;

    // Sink 级别 vs Logger 级别:
    //   - Logger 级别: 第一道过滤，低于此级别的日志直接跳过，不会到达任何 Sink
    //   - Sink 级别: 第二道过滤，每个 Sink 可以独立设置自己的最低接收级别
    //   - 两层过滤的关系: 实际输出 = max(Logger级别, Sink级别)

    // 控制台 Sink: 只显示 warn 及以上（warn, error, critical）
    // 控制台只关注重要事件，避免大量 info/debug 刷屏
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::warn);

    // 文件 Sink: 记录所有级别（trace 及以上）
    // 完整日志用于事后分析和问题排查
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/all_levels.log", true);
    file_sink->set_level(spdlog::level::trace);

    // 错误文件 Sink: 只记录 error 及以上（error, critical）
    // 将错误日志单独归档，便于快速定位问题
    auto error_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/errors.log", true);
    error_sink->set_level(spdlog::level::err);

    // 创建 Logger 并绑定三个 Sink
    spdlog::logger logger("multi_level", {console_sink, file_sink, error_sink});

    // Logger 级别设为 trace（最低），让所有日志都能通过第一道过滤
    // 具体哪些 Sink 接收由各 Sink 自己的级别决定
    logger.set_level(spdlog::level::trace);

    // 下面每条日志的去向（✓ 表示该 Sink 会接收）:
    //                                    all_levels.log   errors.log   console
    logger.trace("trace: only in all_levels.log");      //    ✓
    logger.debug("debug: only in all_levels.log");      //    ✓
    logger.info("info: only in all_levels.log");        //    ✓
    logger.warn("warn: in all_levels.log + console");   //    ✓                          ✓
    logger.error("error: in all_levels.log + errors.log + console");  //  ✓        ✓      ✓
    logger.critical("critical: in all_levels.log + errors.log + console"); // ✓   ✓      ✓

    spdlog::drop("multi_level");
}

int main() {
    try {
        std::cout << "=== Sink 概念演示 ===" << std::endl;

        // 依次运行四个演示函数
        console_sink_demo();            // 1. 控制台 Sink（stdout / stderr）
        file_sink_demo();               // 2. 文件 Sink（basic / rotating / daily）
        null_sink_demo();               // 3. 空 Sink（丢弃日志）
        multi_sink_with_levels_demo();  // 4. 多 Sink + 级别过滤（日志分流）

        std::cout << "\n=== Sink 演示完成 ===" << std::endl;

        // 关闭所有 Logger，刷新所有 Sink 的缓冲区
        // 对文件 Sink 尤其重要，确保缓冲区中的日志数据全部写入磁盘
        spdlog::shutdown();

    } catch (const spdlog::spdlog_ex &ex) {
        // 常见异常: 文件路径不存在或无写入权限（file_sink 初始化失败）
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
