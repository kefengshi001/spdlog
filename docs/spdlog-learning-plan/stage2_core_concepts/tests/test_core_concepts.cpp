// ============================================================================
// spdlog 阶段 2: 核心概念测试
// ============================================================================
// 本文件用于验证 spdlog 的四大核心概念：
//   1. Logger（日志器）—— 日志的核心入口，负责接收日志消息并分发给 Sink
//   2. Sink（输出目标）—— 决定日志输出到哪里（控制台、文件、网络等）
//   3. Formatter（格式化器）—— 决定日志的外观格式（时间戳、级别、消息等）
//   4. Async Logger（异步日志器）—— 通过线程池实现非阻塞日志，提升性能
//
// spdlog 的核心架构模型：
//   用户代码 → Logger → Formatter(格式化) → Sink(s)(输出到目标)
//
// 一个 Logger 可以绑定多个 Sink（multi-sink），实现同时输出到多个目标
// ============================================================================

#include "spdlog/spdlog.h"                    // spdlog 主头文件，包含核心 API
#include "spdlog/sinks/stdout_color_sinks.h"  // 彩色控制台输出 Sink（带 ANSI 颜色）
#include "spdlog/sinks/basic_file_sink.h"     // 基础文件 Sink（写入单个文件）
#include "spdlog/sinks/rotating_file_sink.h"  // 轮转文件 Sink（按大小自动轮转）
#include "spdlog/sinks/null_sink.h"           // 空 Sink（丢弃所有日志，用于测试/基准）
#include "spdlog/async.h"                     // 异步日志支持（线程池 + 异步 Logger）

#include <cassert>   // 断言宏，用于自动化测试验证
#include <fstream>   // 文件流，用于读取日志文件内容进行验证
#include <iostream>  // 标准输出，用于打印测试结果
#include <memory>    // 智能指针（shared_ptr），spdlog Sink 通常以 shared_ptr 管理
#include <string>

// LOG_DIR 宏：日志文件输出目录，可通过 CMake 编译时定义（-DLOG_DIR=...）
// 如果未定义，默认使用当前目录 "."
#ifndef LOG_DIR
#define LOG_DIR "."
#endif

// ============================================================================
// 测试 1: Logger 创建（Logger Creation）
// ============================================================================
// 核心知识点：
//   - spdlog 提供全局工厂函数来创建 Logger，创建后会自动注册到全局注册表
//   - stdout_color_mt()：创建一个带颜色的控制台 Logger，后缀 _mt 表示线程安全（multi-thread）
//   - _st 后缀表示单线程版本（性能更高，但不能跨线程使用）
//   - Logger 创建后会被自动注册，可通过 spdlog::get("name") 获取
//   - 使用完毕后必须 spdlog::drop("name") 从注册表移除，否则会内存泄漏
// ============================================================================
void test_logger_creation() {
    // 创建一个名为 "test_logger" 的彩色控制台 Logger
    // _mt = multi-thread safe（线程安全版本，内部有互斥锁）
    // 返回 std::shared_ptr<spdlog::logger>
    auto logger = spdlog::stdout_color_mt("test_logger");

    // 验证 Logger 创建成功（非空指针）
    assert(logger != nullptr);
    // 验证 Logger 名称正确
    assert(logger->name() == "test_logger");

    // 输出一条 INFO 级别的日志，验证基本功能正常
    logger->info("Logger creation test");

    // 从全局注册表中移除该 Logger
    // 重要：如果不 drop，Logger 会一直存在于全局注册表中
    // 在 spdlog::shutdown() 时会被一起清理，但显式 drop 是好习惯
    spdlog::drop("test_logger");

    std::cout << "  [PASS] logger creation" << std::endl;
}

// ============================================================================
// 测试 2: 日志级别（Log Levels）
// ============================================================================
// 核心知识点：
//   spdlog 定义了 6 个日志级别（从低到高）：
//     trace    → 最详细的调试信息，通常只在开发时开启
//     debug    → 调试信息
//     info     → 一般信息（默认级别）
//     warn     → 警告信息
//     error    → 错误信息
//     critical → 严重错误
//
//   日志级别过滤机制：
//     Logger 和 Sink 都有各自的级别设置
//     只有当日志级别 >= Logger 级别 AND >= Sink 级别时，日志才会被输出
//     这是双重过滤机制，确保精细控制
// ============================================================================
void test_log_levels() {
    auto logger = spdlog::stdout_color_mt("test_levels");

    // 设置 Logger 的日志级别为 debug
    // 这意味着 trace 级别的日志会被 Logger 层面过滤掉
    logger->set_level(spdlog::level::debug);

    // 测试所有日志级别，确保不会崩溃
    // trace 会被过滤（因为 Logger 级别是 debug），但调用本身不应崩溃
    logger->trace("trace");       // 会被过滤（trace < debug）
    logger->debug("debug");       // 会输出（debug >= debug）
    logger->info("info");         // 会输出（info > debug）
    logger->warn("warn");         // 会输出
    logger->error("error");       // 会输出
    logger->critical("critical"); // 会输出

    spdlog::drop("test_levels");
    std::cout << "  [PASS] log levels" << std::endl;
}

// ============================================================================
// 测试 3: 多 Sink（Multi-Sink）
// ============================================================================
// 核心知识点：
//   - 一个 Logger 可以绑定多个 Sink，实现「一次日志，多处输出」
//   - 这是 spdlog 最强大的设计之一：Logger 持有 Sink 列表，日志会分发给所有 Sink
//   - 常见用法：同时输出到控制台（方便实时查看）和文件（持久化存储）
//   - 每个 Sink 独立进行格式化和级别过滤
//
//   架构示意：
//     logger.info("msg") → Sink1(控制台) → 终端显示
//                        → Sink2(文件)   → 写入磁盘
// ============================================================================
void test_multi_sink() {
    std::string log_path = std::string(LOG_DIR) + "/test_multi_sink.log";

    // 创建两个独立的 Sink
    // stdout_color_sink_mt：彩色控制台 Sink（线程安全）
    // basic_file_sink_mt：基础文件 Sink，第二个参数 true 表示 truncate（截断已有文件）
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path, true);

    // 手动创建 Logger 并传入 Sink 列表（初始化列表语法）
    // 与工厂函数不同，这里不自动注册到全局注册表
    spdlog::logger logger("test_multi", {console_sink, file_sink});

    // 一条日志同时输出到控制台和文件
    logger.info("Multi sink test");

    // flush() 确保所有缓冲的日志被立即写入目标
    // 重要：不 flush 的话，日志可能还在缓冲区，文件中看不到
    logger.flush();

    // 验证文件中确实写入了日志内容
    std::ifstream file(log_path);
    assert(file.is_open());

    std::string content;
    std::getline(file, content);
    // 查找日志消息是否存在于文件内容中
    assert(content.find("Multi sink test") != std::string::npos);
    file.close();

    std::cout << "  [PASS] multi sink" << std::endl;
}

// ============================================================================
// 测试 4: Sink 级别过滤（Sink Level Filter）
// ============================================================================
// 核心知识点：
//   - 每个 Sink 可以独立设置自己的日志级别过滤
//   - 日志输出的条件是：日志级别 >= Logger 级别 AND >= Sink 级别
//   - 这允许精细控制：比如 Logger 接收所有级别，但文件 Sink 只记录 warn 及以上
//
//   本测试场景：
//     Logger 级别 = trace（接受所有日志）
//     Sink 级别   = warn（只输出 warn、error、critical）
//     结果：debug/info 被 Sink 过滤掉，warn/error 会写入文件
// ============================================================================
void test_sink_level_filter() {
    std::string log_path = std::string(LOG_DIR) + "/test_level_filter.log";

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path, true);

    // 在 Sink 层面设置过滤级别为 warn
    // 只有 warn、error、critical 才会被这个 Sink 输出
    file_sink->set_level(spdlog::level::warn);

    spdlog::logger logger("test_filter", {file_sink});

    // Logger 层面设置为 trace，接受所有级别的日志
    // 但最终能否输出还要看 Sink 的过滤
    logger.set_level(spdlog::level::trace);

    // debug/info 会被 Sink 过滤，不会写入文件
    logger.debug("should not appear");
    logger.info("should not appear");

    // warn/error 满足 Sink 的级别要求，会写入文件
    logger.warn("should appear");
    logger.error("should appear");
    logger.flush();

    // 读取文件全部内容进行验证
    // 使用 istreambuf_iterator 一次性读取整个文件
    std::ifstream file(log_path);
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    // 验证 "should not appear" 不存在（被正确过滤）
    assert(content.find("should not appear") == std::string::npos);
    // 验证 "should appear" 存在（通过了过滤）
    assert(content.find("should appear") != std::string::npos);
    file.close();

    std::cout << "  [PASS] sink level filter" << std::endl;
}

// ============================================================================
// 测试 5: 格式化器（Formatter）
// ============================================================================
// 核心知识点：
//   spdlog 使用 pattern 字符串来定义日志格式，常用的格式化标记：
//     %Y → 四位年份（2026）        %m → 月份（01-12）
//     %d → 日期（01-31）           %H → 小时（00-23）
//     %M → 分钟（00-59）           %S → 秒（00-59）
//     %l → 日志级别（info/warn...） %n → Logger 名称
//     %v → 实际日志消息内容         %t → 线程 ID
//     %s → 源文件名                %# → 行号
//     %+   → spdlog 的默认格式（推荐，包含完整信息）
//     %^ ... %$ → 颜色标记对（中间的内容会被着色）
//
//   格式化器可以在 Logger 级别或全局级别设置
// ============================================================================
void test_formatter() {
    auto logger = spdlog::stdout_color_mt("test_format");

    // 自定义格式：[年-月-日 时:分:秒] [级别] 消息
    // 输出示例：[2026-05-14 10:30:45] [info] Format test
    logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
    logger->info("Format test");

    // 另一种格式：[Logger名称] 消息
    // 输出示例：[test_format] Logger name test
    logger->set_pattern("[%n] %v");
    logger->info("Logger name test");

    // 包含源码位置信息 + 颜色高亮的格式：
    //   %s → 源文件名（不含路径）    %# → 行号    %! → 函数名
    //   %^ ... %$ → 颜色范围（中间的文字会被 spdlog 着色，不同日志级别显示不同颜色）
    //
    // 简化技巧：将 logger 设为默认 logger，之后就能直接用短宏：
    //   SPDLOG_INFO(fmt, ...)    — 等价于 SPDLOG_LOGGER_INFO(spdlog::default_logger(), ...)
    //   SPDLOG_WARN(fmt, ...)    — 同理，WARN 级别
    //   SPDLOG_ERROR(fmt, ...)   — 同理，ERROR 级别
    // 这些宏自动通过 __FILE__、__LINE__、__FUNCTION__ 捕获源码位置
    //
    // 输出示例：[2026-05-14 10:30:45 test_core_concepts.cpp:120] [info] Source location test
    logger->set_pattern("[%Y-%m-%d %H:%M:%S %s:%#] [%^%l%$] %v");

    // 设为默认 logger，之后 SPDLOG_INFO / SPDLOG_WARN 等短宏就能直接使用
    spdlog::set_default_logger(logger);
    SPDLOG_WARN("Source location test");

    // spdlog::set_pattern() 是全局设置，会影响所有使用默认格式的 Logger
    // "%+" 是 spdlog 的默认格式，等价于 "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v"
    spdlog::set_pattern("%+");

    spdlog::drop("test_format");
    std::cout << "  [PASS] formatter" << std::endl;
}

// ============================================================================
// 测试 6: 轮转文件（Rotating File）
// ============================================================================
// 核心知识点：
//   rotating_file_sink 实现了日志文件的自动轮转管理：
//     - 当当前文件大小超过 max_size 时，自动创建新文件
//     - 旧文件会被重命名（加后缀 _1, _2, ...）
//     - 最多保留 max_files 个历史文件，超出的会被删除
//
//   参数说明：
//     filename  → 日志文件路径
//     max_size  → 单个文件最大字节数（本测试用 1024 即 1KB，便于触发轮转）
//     max_files → 最多保留的历史文件数量（本测试为 2，即最多 3 个文件：当前 + 2 个历史）
//
//   文件命名示例（max_files=2）：
//     test_rotating.log       ← 当前正在写入的文件
//     test_rotating.1.log     ← 第一次轮转的旧文件
//     test_rotating.2.log     ← 第二次轮转的旧文件（最老的）
//
//   适用场景：生产环境日志管理，防止日志文件无限增长撑爆磁盘
// ============================================================================
void test_rotating_file() {
    std::string log_path = std::string(LOG_DIR) + "/test_rotating.log";

    // 创建轮转文件 Logger
    // 参数：名称、文件路径、单文件最大 1024 字节、最多保留 2 个历史文件
    auto logger = spdlog::rotating_logger_mt("test_rotating", log_path, 1024, 2);

    // 写入 100 条日志，总大小约 2-3KB，会触发轮转
    // fmt::format 风格的格式化：{} 是占位符
    for (int i = 0; i < 100; i++) {
        logger->info("Rotating test message #{}", i);
    }
    logger->flush();

    // 验证日志文件存在
    std::ifstream file(log_path);
    assert(file.is_open());
    file.close();

    spdlog::drop("test_rotating");
    std::cout << "  [PASS] rotating file" << std::endl;
}

// ============================================================================
// 测试 7: 异步日志器（Async Logger）
// ============================================================================
// 核心知识点：
//   异步日志是 spdlog 的重要性能特性，工作原理：
//     1. 用户调用 logger->info() 时，日志消息被放入队列（几乎立即返回）
//     2. 后台线程池中的工作线程从队列取出消息，执行格式化和写入
//     3. 这样用户线程不会被 I/O 操作阻塞
//
//   spdlog::init_thread_pool(queue_size, worker_threads):
//     - queue_size：环形缓冲区大小（必须是 2 的幂），队列满时会阻塞生产者
//     - worker_threads：后台工作线程数量
//
//   async_factory：异步 Logger 的工厂模板参数
//     basic_logger_mt<spdlog::async_factory> 表示创建异步的线程安全 Logger
//
//   适用场景：
//     - 高吞吐量应用（如游戏服务器、交易系统）
//     - 日志 I/O 不应影响主线程性能的场景
//     - 注意：异步日志在程序崩溃时可能丢失未处理的队列消息
// ============================================================================
void test_async_logger() {
    // 初始化全局线程池：队列大小 8192，1 个工作线程
    // 这是一个全局操作，整个进程共享一个线程池
    spdlog::init_thread_pool(8192, 1);

    std::string log_path = std::string(LOG_DIR) + "/test_async.log";

    // 使用 async_factory 创建异步 Logger
    // 模板参数 <spdlog::async_factory> 告诉 spdlog 使用异步模式
    // 第三个参数 true 表示 truncate（截断已有文件）
    auto logger = spdlog::basic_logger_mt<spdlog::async_factory>(
        "test_async", log_path, true);

    // 写入 50 条日志，这些日志会先进入队列，由后台线程异步写入文件
    for (int i = 0; i < 50; i++) {
        logger->info("Async test #{}", i);
    }

    // flush() 会等待队列中的所有消息被处理完毕
    // 在异步模式下，这确保所有日志都已写入文件
    logger->flush();

    // 验证日志文件存在
    std::ifstream file(log_path);
    assert(file.is_open());
    file.close();

    spdlog::drop("test_async");
    std::cout << "  [PASS] async logger" << std::endl;
}

// ============================================================================
// 测试 8: 空 Sink（Null Sink）
// ============================================================================
// 核心知识点：
//   null_sink 会丢弃所有接收到的日志消息，不做任何输出
//   用途：
//     - 性能基准测试：测量日志框架本身的开销（排除 I/O）
//     - 禁用日志：在某些环境下不想输出任何日志
//     - 测试占位：需要 Logger 对象但不需要实际输出时
//
//   spdlog::create<SinkType>() 工厂函数：
//     直接用指定的 Sink 类型创建 Logger，比手动构造更简洁
// ============================================================================
void test_null_sink() {
    // 使用 create 工厂函数创建带 null_sink 的 Logger
    // 所有日志都会被静默丢弃
    auto logger = spdlog::create<spdlog::sinks::null_sink_mt>("test_null");

    // 这条日志不会有任何输出，但不应崩溃
    logger->info("This should be discarded");

    spdlog::drop("test_null");
    std::cout << "  [PASS] null sink" << std::endl;
}

// ============================================================================
// 主函数：运行所有测试
// ============================================================================
// 测试执行顺序：
//   1. Logger 创建 → 验证基本生命周期
//   2. 日志级别   → 验证级别过滤机制
//   3. 多 Sink    → 验证一对多输出
//   4. Sink 过滤  → 验证 Sink 级别的独立过滤
//   5. 格式化器   → 验证自定义输出格式
//   6. 轮转文件   → 验证日志文件管理
//   7. 异步日志   → 验证异步写入机制
//   8. 空 Sink    → 验证日志丢弃场景
//
// spdlog::shutdown()：关闭全局资源（线程池、全局注册表等）
// 必须在所有 Logger 使用完毕后调用，否则可能导致未定义行为
// ============================================================================
int main() {
    std::cout << "=== 核心概念测试 ===" << std::endl;

    test_logger_creation();   // Logger 基本创建与销毁
    test_log_levels();        // 日志级别过滤
    test_multi_sink();        // 多 Sink 同时输出
    test_sink_level_filter(); // Sink 级别的独立过滤
    test_formatter();         // 自定义日志格式
    test_rotating_file();     // 轮转文件管理
    test_async_logger();      // 异步日志
    test_null_sink();         // 空 Sink 丢弃

    // 关闭 spdlog 全局资源
    // 这会：1. 刷新所有 Logger  2. 关闭线程池  3. 清空全局注册表
    spdlog::shutdown();

    std::cout << "\n=== 所有测试通过 ===" << std::endl;
    return 0;
}
