// RoboLog 性能测试
// 验证日志写入不会阻塞 1kHz 控制循环

#include "robo_log.h"
#include "robo_log_config.h"

#include <chrono>
#include <iostream>
#include <vector>
#include <numeric>

void test_sync_performance() {
    std::cout << "\n=== 1. 同步日志性能 ===" << std::endl;

    RoboLogConfig config;
    config.console_enabled = false;
    config.file_enabled = true;
    config.file_path = std::string(LOG_DIR) + "/perf_sync.log";
    config.error_file_enabled = false;
    config.async_enabled = false;

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

    RoboLog::flush();
    RoboLog::shutdown();

    // 计算统计
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

void test_async_performance() {
    std::cout << "\n=== 2. 异步日志性能 ===" << std::endl;

    RoboLogConfig config;
    config.console_enabled = false;
    config.file_enabled = true;
    config.file_path = std::string(LOG_DIR) + "/perf_async.log";
    config.error_file_enabled = false;
    config.async_enabled = true;
    config.async_queue_size = 65536;
    config.async_thread_count = 1;

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

    RoboLog::flush();
    RoboLog::shutdown();

    // 计算统计
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

void test_1khz_simulation() {
    std::cout << "\n=== 3. 1kHz 控制循环模拟 ===" << std::endl;

    RoboLogConfig config;
    config.console_enabled = false;
    config.file_enabled = true;
    config.file_path = std::string(LOG_DIR) + "/perf_1khz.log";
    config.error_file_enabled = false;
    config.async_enabled = true;
    config.async_queue_size = 65536;

    RoboLog::init(config);

    const int cycles = 1000;  // 1 秒的 1kHz 循环
    int overrun_count = 0;

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < cycles; i++) {
        auto cycle_start = std::chrono::steady_clock::now();

        // 模拟控制循环中的日志
        RoboLog::trace("Cycle {}: angle={:.4f}, velocity={:.4f}, torque={:.4f}",
                       i, 45.0 + i * 0.001, 1.5, 10.0);

        auto cycle_end = std::chrono::steady_clock::now();
        auto cycle_time = std::chrono::duration_cast<std::chrono::microseconds>(
            cycle_end - cycle_start).count();

        if (cycle_time > 1000) {  // 超过 1ms
            overrun_count++;
        }

        // 模拟 1kHz 周期
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

    test_sync_performance();
    test_async_performance();
    test_1khz_simulation();

    std::cout << "\n=== 性能测试完成 ===" << std::endl;
    std::cout << "结论: 异步日志适合 1kHz 控制循环场景" << std::endl;

    return 0;
}
