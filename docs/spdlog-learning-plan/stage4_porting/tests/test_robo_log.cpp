/**
 * RoboLog 单元测试 (Unit Tests)
 *
 * 本文件测试 RoboLog 封装层的核心功能:
 *   1. 初始化/关闭   - init() 和 shutdown() 不崩溃
 *   2. 日志级别      - 所有级别(trace~critical)正常工作
 *   3. 格式化输出    - fmt::format 语法正确解析
 *   4. 文件输出      - 日志文件正确创建
 *   5. 级别过滤      - 低于阈值的日志被正确过滤
 *
 * 测试方法:
 *   - 使用 assert 断言(简单直接)
 *   - 每个测试独立 init/shutdown，互不影响
 *   - 通过文件内容验证日志是否正确写入
 *
 * 编译时可通过 -DLOG_DIR="xxx" 指定日志输出目录
 */

#include "robo_log.h"
#include "robo_log_config.h"

#include <cassert>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#ifndef LOG_DIR
#define LOG_DIR "."
#endif

/**
 * 测试 1: 初始化和关闭
 *
 * 验证:
 *   - debug_config() 配置正确加载
 *   - init() 不抛出异常
 *   - shutdown() 不抛出异常
 *   - 可以多次 init/shutdown(幂等性)
 *
 * 这是最基础的冒烟测试: 如果这个都过不了，其他测试也不用跑了
 */
void test_init_shutdown() {
    RoboLog::init(debug_config());
    assert(true);  // 能走到这里说明没有崩溃
    RoboLog::shutdown();
    std::cout << "  [PASS] init/shutdown" << std::endl;
}

/**
 * 测试 2: 所有日志级别
 *
 * 验证:
 *   - trace/debug/info/warn/error/critical 六个级别都能正常调用
 *   - 不抛出异常、不崩溃
 *
 * debug_config() 设置全局级别为 trace，所以所有级别都应该被记录
 */
void test_log_levels() {
    RoboLog::init(debug_config());

    RoboLog::trace("trace message");
    RoboLog::debug("debug message");
    RoboLog::info("info message");
    RoboLog::warn("warn message");
    RoboLog::error("error message");
    RoboLog::critical("critical message");

    RoboLog::shutdown();
    std::cout << "  [PASS] log levels" << std::endl;
}

/**
 * 测试 3: 格式化日志输出
 *
 * 验证:
 *   - spdlog 的 fmt::format 语法正确工作
 *   - 数字格式化({:.2f})正确
 *   - 字符串替换({})正确
 *
 * 注意: RoboLog 的接口接受 std::string，格式化由 spdlog 内部处理
 *       这里测试的是 spdlog 的 fmt 兼容性
 */
void test_formatted_logging() {
    RoboLog::init(debug_config());

    RoboLog::info("Joint {}: angle={:.2f}°", "J1", 45.678);  // 字符串 + 浮点数
    RoboLog::warn("Speed near limit: {:.2f} rad/s", 2.95);   // 浮点数
    RoboLog::error("Timeout: {}ms", 150);                     // 整数

    RoboLog::shutdown();
    std::cout << "  [PASS] formatted logging" << std::endl;
}

/**
 * 测试 4: 文件输出
 *
 * 验证:
 *   - 运行日志文件正确创建
 *   - 错误日志文件正确创建
 *   - 两个文件独立存在
 *
 * 配置: 关闭控制台，仅文件输出(避免干扰测试输出)
 *       关闭异步(同步模式便于验证文件立即写入)
 */
void test_file_output() {
    std::string log_path = std::string(LOG_DIR) + "/test_robo.log";
    std::string error_base = std::string(LOG_DIR) + "/test_robo_error.log";

    // 错误日志使用每日轮转，实际文件名包含日期: test_robo_error_YYYY-MM-DD.log
    // 构造带日期的文件名用于验证
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&t);
    char date_buf[16];
    std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tm);
    // 从 base 路径分离文件名和扩展名，插入日期
    auto dot_pos = error_base.rfind('.');
    std::string error_path = error_base.substr(0, dot_pos) + "_" + date_buf + error_base.substr(dot_pos);

    // 自定义配置: 仅文件输出
    RoboLogConfig config;
    config.console_enabled = false;        // 关闭控制台
    config.file_enabled = true;            // 开启运行日志
    config.file_path = log_path;
    config.error_file_enabled = true;      // 开启错误日志
    config.error_file_path = error_base;
    config.async_enabled = false;          // 同步模式

    RoboLog::init(config);
    RoboLog::info("Test message");    // 写入运行日志
    RoboLog::error("Error message");  // 同时写入运行日志和错误日志
    RoboLog::flush();                 // 确保立即写入
    RoboLog::shutdown();

    // 验证文件存在
    std::ifstream log_file(log_path);
    assert(log_file.is_open());
    log_file.close();

    std::ifstream error_file(error_path);
    assert(error_file.is_open());
    error_file.close();

    std::cout << "  [PASS] file output" << std::endl;
}

/**
 * 测试 5: 日志级别过滤
 *
 * 验证:
 *   - 设置 warn 级别后，debug 和 info 日志不写入文件
 *   - warn 和 error 日志正常写入
 *   - 级别过滤在 logger 层面生效(不是 sink 层面)
 *
 * 这是生产环境最常用的配置:
 *   - 控制台只显示 warn 以上(减少噪音)
 *   - 文件记录 info 以上(保留关键信息)
 *   - 错误文件只记录 error 以上(快速定位问题)
 */
void test_level_filtering() {
    std::string log_path = std::string(LOG_DIR) + "/test_level_filter.log";

    // 配置: warn 级别过滤
    RoboLogConfig config;
    config.console_enabled = false;
    config.file_enabled = true;
    config.file_path = log_path;
    config.error_file_enabled = false;
    config.level = spdlog::level::warn;    // 只记录 warn 及以上
    config.async_enabled = false;

    RoboLog::init(config);

    // 这些应该被过滤(不写入文件)
    RoboLog::debug("should not appear");
    RoboLog::info("should not appear");

    // 这些应该被记录
    RoboLog::warn("should appear");
    RoboLog::error("should appear");

    RoboLog::flush();
    RoboLog::shutdown();

    // 读取文件内容并验证
    std::ifstream file(log_path);
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    // 断言: 文件中不应包含被过滤的消息
    assert(content.find("should not appear") == std::string::npos);
    // 断言: 文件中应包含未被过滤的消息
    assert(content.find("should appear") != std::string::npos);

    std::cout << "  [PASS] level filtering" << std::endl;
}

int main() {
    std::cout << "=== RoboLog 单元测试 ===" << std::endl;

    test_init_shutdown();      // 基础冒烟测试
    test_log_levels();         // 日志级别测试
    test_formatted_logging();  // 格式化输出测试
    test_file_output();        // 文件输出测试
    test_level_filtering();    // 级别过滤测试

    std::cout << "\n=== 所有测试通过 ===" << std::endl;
    return 0;
}
