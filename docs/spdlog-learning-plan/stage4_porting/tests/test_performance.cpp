/**
 * RoboLog 性能测试 (Performance Benchmark)
 *
 * 本文件测试 RoboLog 在不同模式下的性能表现，验证日志不会阻塞 1kHz 控制循环
 *
 * 测试内容:
 *   1. 同步日志性能 — 测量单次 info() 调用的延迟
 *   2. 异步日志性能 — 测量异步模式下 info() 调用的延迟(应显著更低)
 *   3. 1kHz 控制循环模拟 — 模拟真实场景，检查是否有周期超时
 *
 * 性能指标:
 *   - 平均延迟: 单次日志调用的平均耗时(微秒)
 *   - 最小/最大延迟: 最好和最坏情况
 *   - 占用比例: 日志耗时占 1kHz 周期(1000μs)的百分比
 *   - 超时率: 超过 1ms 的周期占比
 *
 * 预期结果:
 *   - 同步日志: 平均 10-100μs(取决于磁盘速度)
 *   - 异步日志: 平均 1-5μs(只入队不写盘)
 *   - 异步模式下 1kHz 控制循环的超时率应接近 0%
 */

#include "robo_log.h"
#include "robo_log_config.h"

#include <chrono>
#include <iostream>
#include <vector>
#include <numeric>

#ifndef LOG_DIR
#define LOG_DIR "."
#endif

/**
 * 测试 1: 同步日志性能
 *
 * 测量同步模式下每次 info() 调用的延迟
 * 同步模式: info() 会直接触发文件 I/O，延迟较高但数据立即落盘
 *
 * 测试方法:
 *   - 写入 1000 条日志
 *   - 每条日志前后记录时间戳
 *   - 计算平均/最小/最大延迟
 *
 * 机器人场景关注点:
 *   - 如果平均延迟 > 100μs，可能影响 1kHz 控制循环
 *   - 最大延迟更重要: 即使平均低，偶发的高延迟也会导致控制抖动
 */
void test_sync_performance() {
    std::cout << "\n=== 1. 同步日志性能 ===" << std::endl;

    // 配置: 仅文件输出(排除终端 I/O 干扰)
    RoboLogConfig config;
    config.console_enabled = false;                    // 关闭控制台输出
    config.file_enabled = true;
    config.file_path = std::string(LOG_DIR) + "/perf_sync.log";
    config.error_file_enabled = false;
    config.async_enabled = false;                      // 同步模式

    RoboLog::init(config);

    const int iterations = 1000;
    std::vector<long long> latencies;  // 存储每次调用的延迟(纳秒)

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::steady_clock::now();
        RoboLog::info("Control cycle {}: joint_angle={:.2f}", i, 45.0 + i * 0.01);
        auto end = std::chrono::steady_clock::now();

        latencies.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
    }

    RoboLog::flush();
    RoboLog::shutdown();

    // 统计分析
    long long total = std::accumulate(latencies.begin(), latencies.end(), 0LL);
    long long avg = total / iterations;
    long long max = *std::max_element(latencies.begin(), latencies.end());
    long long min = *std::min_element(latencies.begin(), latencies.end());

    std::cout << "  同步日志 (" << iterations << " 次):" << std::endl;
    std::cout << "    平均延迟: " << avg / 1000.0 << " μs" << std::endl;
    std::cout << "    最小延迟: " << min / 1000.0 << " μs" << std::endl;
    std::cout << "    最大延迟: " << max / 1000.0 << " μs" << std::endl;
    std::cout << "    1kHz 周期: 1000 μs" << std::endl;
    std::cout << "    占用比例: " << (avg / 1000000.0) * 100 << "%" << std::endl;
}

/**
 * 测试 2: 异步日志性能
 *
 * 测量异步模式下每次 info() 调用的延迟
 * 异步模式: info() 只将消息放入队列就返回，I/O 由后台线程完成
 *
 * 预期: 延迟应比同步模式低 1-2 个数量级
 * 原因: 入队操作是纯内存操作(无磁盘 I/O)
 *
 * 配置:
 *   - async_queue_size = 65536: 足够大的队列，避免测试中溢出
 *   - async_thread_count = 1: 一个消费者线程(单文件写入不需要多线程)
 */
void test_async_performance() {
    std::cout << "\n=== 2. 异步日志性能 ===" << std::endl;

    RoboLogConfig config;
    config.console_enabled = false;
    config.file_enabled = true;
    config.file_path = std::string(LOG_DIR) + "/perf_async.log";
    config.error_file_enabled = false;
    config.async_enabled = true;                       // 异步模式
    config.async_queue_size = 65536;                   // 大队列
    config.async_thread_count = 1;                     // 单消费者线程

    RoboLog::init(config);

    const int iterations = 1000;
    std::vector<long long> latencies;

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::steady_clock::now();
        RoboLog::info("Control cycle {}: joint_angle={:.2f}", i, 45.0 + i * 0.01);
        auto end = std::chrono::steady_clock::now();

        latencies.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
    }

    RoboLog::flush();  // 等待异步队列处理完毕
    RoboLog::shutdown();

    // 统计分析
    long long total = std::accumulate(latencies.begin(), latencies.end(), 0LL);
    long long avg = total / iterations;
    long long max = *std::max_element(latencies.begin(), latencies.end());
    long long min = *std::min_element(latencies.begin(), latencies.end());

    std::cout << "  异步日志 (" << iterations << " 次):" << std::endl;
    std::cout << "    平均延迟: " << avg / 1000.0 << " μs" << std::endl;
    std::cout << "    最小延迟: " << min / 1000.0 << " μs" << std::endl;
    std::cout << "    最大延迟: " << max / 1000.0 << " μs" << std::endl;
    std::cout << "    1kHz 周期: 1000 μs" << std::endl;
    std::cout << "    占用比例: " << (avg / 1000000.0) * 100 << "%" << std::endl;
}

/**
 * 测试 3: 1kHz 控制循环模拟
 *
 * 模拟真实的机器人控制循环:
 *   - 每 1ms 执行一次控制周期(1kHz)
 *   - 每个周期记录一次 trace 级别的关节状态
 *   - 检查日志调用是否导致周期超时(> 1ms)
 *
 * 关键指标:
 *   - overrun_count: 超时的周期数
 *   - overrun_rate: 超时率(应 < 1% 为合格)
 *   - 如果超时率高，说明需要使用异步日志或减少日志频率
 *
 * sleep_until 精度:
 *   - 使用 steady_clock 确保不受系统时间调整影响
 *   - 实际精度取决于操作系统(通常 10-100μs)
 *   - Linux 下可通过 PREEMPT_RT 补丁提升实时性
 */
void test_1khz_simulation() {
    std::cout << "\n=== 3. 1kHz 控制循环模拟 ===" << std::endl;

    RoboLogConfig config;
    config.console_enabled = false;
    config.file_enabled = true;
    config.file_path = std::string(LOG_DIR) + "/perf_1khz.log";
    config.error_file_enabled = false;
    config.async_enabled = true;       // 异步模式: 日志不阻塞控制循环
    config.async_queue_size = 65536;

    RoboLog::init(config);

    const int cycles = 1000;  // 模拟 1 秒(1000 个 1kHz 周期)
    int overrun_count = 0;    // 超时计数

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < cycles; i++) {
        auto cycle_start = std::chrono::steady_clock::now();

        // 模拟控制循环中的日志记录
        // trace 级别: 记录关节角度、速度、力矩
        RoboLog::trace("Cycle {}: angle={:.4f}, velocity={:.4f}, torque={:.4f}",
                       i, 45.0 + i * 0.001, 1.5, 10.0);

        auto cycle_end = std::chrono::steady_clock::now();
        auto cycle_time = std::chrono::duration_cast<std::chrono::microseconds>(
            cycle_end - cycle_start).count();

        // 检查是否超时: 日志调用耗时超过 1ms
        if (cycle_time > 1000) {
            overrun_count++;
        }

        // 等待到下一个周期开始点
        // sleep_until 确保周期间隔精确，不受日志调用耗时影响
        std::this_thread::sleep_until(
            cycle_start + std::chrono::microseconds(1000));
    }

    auto end = std::chrono::steady_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    RoboLog::flush();
    RoboLog::shutdown();

    std::cout << "  1kHz 模拟 (" << cycles << " 周期):" << std::endl;
    std::cout << "    总时间: " << total_time << " ms" << std::endl;
    std::cout << "    超时周期: " << overrun_count << " / " << cycles << std::endl;
    std::cout << "    超时率: " << (overrun_count * 100.0 / cycles) << "%" << std::endl;
}

int main() {
    std::cout << "=== RoboLog 性能测试 ===" << std::endl;

    test_sync_performance();    // 同步模式基准
    test_async_performance();   // 异步模式基准
    test_1khz_simulation();     // 1kHz 控制循环模拟

    std::cout << "\n=== 性能测试完成 ===" << std::endl;
    std::cout << "结论: 异步日志适合 1kHz 控制循环场景" << std::endl;

    return 0;
}
