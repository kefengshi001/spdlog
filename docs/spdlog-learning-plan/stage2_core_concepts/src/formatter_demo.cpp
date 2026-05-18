// spdlog 阶段 2: Formatter 概念演示
// 演示 pattern 语法和自定义格式
//
// spdlog 的 Formatter 负责将日志事件格式化为最终的字符串输出。
// 用户通过 set_pattern() 设置格式模板，模板由普通字符和以 % 开头的标志(flag)组成。
// 格式模板语法类似 strftime，但扩展了日志特有的标志。

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <iostream>

// 演示 spdlog pattern 标志的使用
// pattern 标志是以 % 开头的特殊占位符，在输出时会被替换为对应的值
void pattern_flags_demo() {
    std::cout << "\n=== 1. Pattern 标志 ===" << std::endl;

    // 创建一个名为 "pattern_demo" 的控制台 logger（带颜色输出）
    auto logger = spdlog::stdout_color_mt("pattern_demo");

    // 常用 pattern 标志:
    // %Y-%m-%d : 日期 (2026-05-13)
    // %H:%M:%S : 时间 (22:09:25)
    // %e : 毫秒
    // %l : 日志级别 (info, debug, etc.)
    // %^ : 颜色开始标记（仅对带颜色的 sink 生效）
    // %$ : 颜色结束标记
    // %v : 日志消息正文（即 logger->info("...") 中的字符串）
    // %t : 线程 ID（哈希值形式）
    // %s : 源文件名（仅当使用 SPDLOG_DEBUG 等宏时可用）
    // %# : 源行号（仅当使用 SPDLOG_DEBUG 等宏时可用）
    // %! : 函数名（仅当使用 SPDLOG_DEBUG 等宏时可用）
    // %n : Logger 名称（创建 logger 时传入的名称）
    // %P : 进程 ID
    // %i : 线程 ID（数字形式）

    // 示例 1: 带时间戳、颜色级别和线程 ID 的格式
    // 输出形如: [2026-05-13 22:09:25.123] [info] [12345] Pattern with timestamp and thread ID
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
    logger->info("Pattern with timestamp and thread ID");

    // 示例 2: 带 Logger 名称的格式
    // %n 会被替换为创建 logger 时的名称 "pattern_demo"
    logger->set_pattern("[%H:%M:%S] [%n] [%^%l%$] %v");
    logger->info("Pattern with logger name");

    spdlog::set_pattern("[%H:%M:%S.%e] [%l] [%@] %v");
    SPDLOG_INFO("This will show full path and line");

    // 示例 3: 带源文件位置信息的格式
    // 注意: %s %# %! 只有通过宏(如 SPDLOG_LOGGER_CALL)调用时才能获取到正确值
    // 直接调用 logger->info() 时这些字段为空
    logger->set_pattern("[%l] %s:%# %! - %v");
    logger->info("Pattern with source location");

    // 使用完毕后从全局注册表中移除该 logger，避免资源泄漏
    spdlog::drop("pattern_demo");
}

// 演示 pattern 中的对齐和填充功能
// 通过在 % 后面添加数字可以控制字段宽度，使用 - 表示左对齐
// 这对于输出对齐的表格样式日志非常有用
void alignment_demo() {
    std::cout << "\n=== 2. 对齐和填充 ===" << std::endl;

    auto logger = spdlog::stdout_color_mt("align_demo");

    // 右对齐，8 字符宽（默认行为）
    // 级别字符串不足 8 字符时在左侧补空格
    // 例如: [     info] Right aligned
    logger->set_pattern("[%8l] %v");
    logger->info("Right aligned");

    // 左对齐，8 字符宽（- 表示左对齐）
    // 级别字符串不足 8 字符时在右侧补空格
    // 例如: [info     ] Left aligned
    logger->set_pattern("[%-8l] %v");
    logger->info("Left aligned");

    spdlog::drop("align_demo");
}

// 演示机械臂控制系统推荐的日志格式
// 结合了时间戳、级别、线程 ID 和格式化数值输出
// 对于实时控制系统，精确的时间戳和清晰的级别标识至关重要
void robot_format_demo() {
    std::cout << "\n=== 3. 机械臂推荐格式 ===" << std::endl;

    auto logger = spdlog::stdout_color_mt("robot_format");

    // 机械臂控制系统推荐格式:
    // - 毫秒级时间戳: 用于精确追踪事件时序
    // - 左对齐 8 字符的彩色级别: 便于快速扫描
    // - 线程 ID: 多线程环境下定位问题
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%-8l%$] [%t] %v");

    // {} 是 spdlog/fmt 的格式化占位符，{:.2f} 表示保留 2 位小数的浮点数
    logger->info("Joint angle: {:.2f} deg", 45.678);       // 正常运行信息
    logger->warn("Joint speed near limit: {:.2f} rad/s", 2.95);  // 警告: 接近速度限制
    logger->error("Communication timeout: {}ms", 150);      // 错误: 通信超时
    logger->critical("Collision detected, emergency stop");  // 严重: 碰撞检测，紧急停止

    spdlog::drop("robot_format");
}

// 演示 spdlog 的格式化值功能
// spdlog 内部使用 fmt 库进行格式化，语法与 Python 的 str.format() 或 C++20 std::format 类似
// {} 是占位符，按顺序匹配后面的参数
void format_values_demo() {
    std::cout << "\n=== 4. 格式化值 ===" << std::endl;

    auto logger = spdlog::stdout_color_mt("format_values");
    logger->set_pattern("[%H:%M:%S] [%l] %v");

    // 基本格式化: {} 会被依次替换为后面的参数
    logger->info("String: {}", "hello");         // 字符串: hello
    logger->info("Integer: {}", 42);             // 整数: 42
    logger->info("Float: {:.3f}", 3.14159);      // 浮点数，:.3f 表示保留 3 位小数: 3.142

    // 多个参数: 占位符按顺序匹配，支持不同类型的参数混用
    logger->info("Joint {}: angle={:.2f}, speed={:.2f}", "J1", 45.0, 1.5);

    // 填充和对齐（在占位符内部控制输出格式）:
    // {:>10} 表示右对齐，总宽度 10 字符
    logger->info("Padded: {:>10}", "right");     // 输出: "     right"
    // {:<10} 表示左对齐，总宽度 10 字符
    logger->info("Padded: {:<10}", "left");      // 输出: "left      "

    spdlog::drop("format_values");
}

int main() {
    try {
        std::cout << "=== Formatter 概念演示 ===" << std::endl;

        // 依次运行四个演示函数
        pattern_flags_demo();   // 1. pattern 标志
        alignment_demo();       // 2. 对齐和填充
        robot_format_demo();    // 3. 机械臂推荐格式
        format_values_demo();   // 4. 格式化值

        // %+ 是 spdlog 的默认 pattern，等价于: [YYYY-MM-DD HH:MM:SS.mmm] [level] message
        // 演示结束后恢复默认格式，确保后续使用默认 logger 时输出正常
        spdlog::set_pattern("%+");

        std::cout << "\n=== Formatter 演示完成 ===" << std::endl;

        // 关闭所有 logger，刷新缓冲区，释放资源
        // 程序结束前调用确保所有日志都已写出
        spdlog::shutdown();

    } catch (const spdlog::spdlog_ex &ex) {
        // spdlog 的异常类型，通常在底层 I/O 出错时抛出
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
