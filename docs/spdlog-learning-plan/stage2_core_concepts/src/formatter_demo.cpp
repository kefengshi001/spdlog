// spdlog 阶段 2: Formatter 概念演示
// 演示 pattern 语法和自定义格式

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <iostream>

void pattern_flags_demo() {
    std::cout << "\n=== 1. Pattern 标志 ===" << std::endl;

    auto logger = spdlog::stdout_color_mt("pattern_demo");

    // 常用 pattern 标志:
    // %Y-%m-%d : 日期 (2026-05-13)
    // %H:%M:%S : 时间 (22:09:25)
    // %e : 毫秒
    // %l : 日志级别 (info, debug, etc.)
    // %^ : 颜色开始
    // %$ : 颜色结束
    // %v : 日志消息
    // %t : 线程 ID
    // %s : 源文件名
    // %# : 源行号
    // %! : 函数名
    // %n : Logger 名称
    // %P : 进程 ID
    // %i : 线程 ID (数字)

    // 带时间戳和线程 ID
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
    logger->info("Pattern with timestamp and thread ID");

    // 带 Logger 名称
    logger->set_pattern("[%H:%M:%S] [%n] [%^%l%$] %v");
    logger->info("Pattern with logger name");

    // 带源文件和行号
    logger->set_pattern("[%l] %s:%# %! - %v");
    logger->info("Pattern with source location");

    spdlog::drop("pattern_demo");
}

void alignment_demo() {
    std::cout << "\n=== 2. 对齐和填充 ===" << std::endl;

    auto logger = spdlog::stdout_color_mt("align_demo");

    // 右对齐，8 字符宽
    logger->set_pattern("[%8l] %v");
    logger->info("Right aligned");

    // 左对齐，8 字符宽
    logger->set_pattern("[%-8l] %v");
    logger->info("Left aligned");

    spdlog::drop("align_demo");
}

void robot_format_demo() {
    std::cout << "\n=== 3. 机械臂推荐格式 ===" << std::endl;

    auto logger = spdlog::stdout_color_mt("robot_format");

    // 机械臂控制系统推荐格式
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%-8l%$] [%t] %v");
    logger->info("Joint angle: {:.2f} deg", 45.678);
    logger->warn("Joint speed near limit: {:.2f} rad/s", 2.95);
    logger->error("Communication timeout: {}ms", 150);
    logger->critical("Collision detected, emergency stop");

    spdlog::drop("robot_format");
}

void format_values_demo() {
    std::cout << "\n=== 4. 格式化值 ===" << std::endl;

    auto logger = spdlog::stdout_color_mt("format_values");
    logger->set_pattern("[%H:%M:%S] [%l] %v");

    // 基本格式化
    logger->info("String: {}", "hello");
    logger->info("Integer: {}", 42);
    logger->info("Float: {:.3f}", 3.14159);

    // 多个参数
    logger->info("Joint {}: angle={:.2f}, speed={:.2f}", "J1", 45.0, 1.5);

    // 填充和对齐
    logger->info("Padded: {:>10}", "right");
    logger->info("Padded: {:<10}", "left");

    spdlog::drop("format_values");
}

int main() {
    try {
        std::cout << "=== Formatter 概念演示 ===" << std::endl;

        pattern_flags_demo();
        alignment_demo();
        robot_format_demo();
        format_values_demo();

        // 恢复默认格式
        spdlog::set_pattern("%+");

        std::cout << "\n=== Formatter 演示完成 ===" << std::endl;
        spdlog::shutdown();

    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
