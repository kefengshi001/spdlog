/**
 * spdlog 阶段 3: 异步日志调优 (Async Logging Tuning)
 *
 * 本文件演示 spdlog 异步日志的核心调优参数，包括:
 *   1. 队列大小 (Queue Size)     - 控制内存占用与吞吐量的平衡
 *   2. 工作线程数 (Thread Count)  - 控制并发写入能力
 *   3. 溢出策略 (Overflow Policy) - 队列满时的行为选择
 *   4. 同步 vs 异步性能对比       - 验证异步日志的性能优势
 *
 * 关键概念:
 *   - spdlog 的异步日志基于 "生产者-消费者" 模型
 *   - 日志调用方(生产者)将消息放入线程安全队列
 *   - 后台工作线程(消费者)从队列取出消息并写入 Sink
 *   - 这使得日志调用方的阻塞时间极短，适合实时性要求高的场景
 */

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"           // 异步日志核心头文件，提供 async_factory 和线程池初始化

#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>

/**
 * 演示 1: 队列大小调优
 *
 * 队列大小决定了异步日志能缓冲多少条待写入的消息:
 *   - 队列太小 → 高频日志时队列容易满，触发溢出策略(阻塞/丢弃)
 *   - 队列太大 → 浪费内存，且 shutdown 时需要更长时间刷新
 *
 * spdlog::init_thread_pool(queue_size, thread_count):
 *   - queue_size: 环形队列的容量(必须是 2 的幂)
 *   - thread_count: 消费者线程数
 *
 * 注意: spdlog 全局线程池只能初始化一次，切换配置需要先 shutdown 再重新初始化
 */
void queue_size_demo() {
    std::cout << "\n=== 1. 队列大小调优 ===" << std::endl;

    // 小队列 (1024 条): 适合日志量少的场景，内存占用最小
    // 每条日志消息约占 200-500 字节，1024 条约 200KB-500KB
    spdlog::init_thread_pool(1024, 1);  // 队列容量 1024，1 个工作线程
    auto small_queue = spdlog::basic_logger_mt<spdlog::async_factory>(
        "small_queue", "logs/small_queue.log", true);

    // 大队列 (65536 条): 适合高频日志场景(如 1kHz 控制循环)
    // 65536 条约 12MB-30MB，能缓冲约 65 秒的 1kHz 日志
    // 注意: 必须先关闭旧的 logger 和线程池，才能重新初始化
    spdlog::shutdown();  // 关闭所有 logger 并刷新缓冲区
    spdlog::init_thread_pool(65536, 1);
    auto large_queue = spdlog::basic_logger_mt<spdlog::async_factory>(
        "large_queue", "logs/large_queue.log", true);

    std::cout << "  小队列(1024): 适合低频日志" << std::endl;
    std::cout << "  大队列(65536): 适合高频日志" << std::endl;

    // drop() 只移除 logger 注册，不影响线程池
    // shutdown() 才会关闭线程池并刷新所有缓冲区
    spdlog::drop("small_queue");
    spdlog::drop("large_queue");
}

/**
 * 演示 2: 工作线程数调优
 *
 * 工作线程负责从队列中取出日志消息并写入 Sink:
 *   - 1 个线程: 适合单文件写入(避免文件锁竞争)
 *   - 多个线程: 适合多 Sink 写入(如同时写文件+网络+数据库)
 *
 * 一般建议:
 *   - 如果只有 1-2 个 Sink，1 个工作线程足够
 *   - 如果有多个独立 Sink(不共享文件)，可以增加线程数
 *   - 线程数不应超过 CPU 核心数
 */
void thread_count_demo() {
    std::cout << "\n=== 2. 线程数调优 ===" << std::endl;

    spdlog::shutdown();
    spdlog::init_thread_pool(8192, 2);  // 队列 8192，2 个工作线程

    auto logger = spdlog::basic_logger_mt<spdlog::async_factory>(
        "multi_thread", "logs/multi_thread.log", true);

    std::cout << "  2 个工作线程: 适合多文件写入场景" << std::endl;
    std::cout << "  1 个工作线程: 适合单文件写入场景" << std::endl;

    spdlog::drop("multi_thread");
}

/**
 * 演示 3: 溢出策略 (Overflow Policy)
 *
 * 当异步队列已满时，spdlog 提供三种策略:
 *
 *   1. block_retry (默认):
 *      - 生产者线程阻塞等待，直到队列有空位
 *      - 最安全: 不会丢失任何日志
 *      - 缺点: 可能阻塞业务线程(如控制循环)
 *
 *   2. discard_log:
 *      - 直接丢弃新来的日志消息
 *      - 最快: 生产者永远不会阻塞
 *      - 缺点: 可能丢失关键日志(如错误信息)
 *
 *   3. overrun_oldest:
 *      - 覆盖队列中最旧的未处理消息
 *      - 折中: 保留最新日志，丢弃旧日志
 *      - 适合: 只关心最近状态的监控场景
 *
 * 设置方式: spdlog::init_thread_pool() 之后，通过全局 AsyncOverflowPolicy 设置
 */
void overflow_policy_demo() {
    std::cout << "\n=== 3. 溢出策略 ===" << std::endl;

    std::cout << "  block_retry: 阻塞等待（默认，最安全）" << std::endl;
    std::cout << "  discard_log: 丢弃新消息（最快）" << std::endl;
    std::cout << "  overrun_oldest: 覆盖最旧消息" << std::endl;

    // 使用丢弃策略: 当队列满时，新日志会被静默丢弃
    // 适合场景: 日志量远超写入速度时，优先保证业务线程不阻塞
    spdlog::shutdown();
    spdlog::init_thread_pool(1024, 1);  // 故意使用小队列来触发溢出

    auto logger = spdlog::basic_logger_mt<spdlog::async_factory>(
        "discard", "logs/discard.log", true);

    // 快速写入 10000 条日志，小队列可能无法全部缓冲
    for (int i = 0; i < 10000; i++) {
        logger->info("Fast logging #{}", i);
    }

    spdlog::drop("discard");
}

/**
 * 演示 4: 同步 vs 异步日志 — 延迟对比
 *
 * 为什么之前的测试同步看起来更快?
 *   因为 spdlog 的 basic_file_sink 内部有 8KB 缓冲区
 *   info() 只是往内存缓冲区拷贝，没有真正写磁盘
 *   等缓冲区满了才触发一次真正的 write() 系统调用
 *
 * 异步日志每次 info() 都要做队列操作(加锁+入队+通知消费者)
 * 所以单次调用确实比"同步缓冲写入"慢
 *
 * 但异步的真正优势是: 避免磁盘 I/O 阻塞调用方
 *   当同步缓冲区满时，info() 会触发磁盘写入，阻塞数毫秒
 *   异步的磁盘 I/O 永远在后台线程，调用方最多等入队
 *
 * 本测试展示两个维度:
 *   1. 平均延迟: 同步(有缓冲) vs 异步(队列操作)
 *   2. 最大延迟: 同步(缓冲区满时的磁盘 I/O) vs 异步(最多等入队)
 */
void performance_comparison() {
    std::cout << "\n=== 4. 性能对比 ===" << std::endl;

    const int iterations = 5000;

    // --- 同步日志测试 ---
    // basic_file_sink 有 8KB 缓冲区，info() 大部分时候只写内存
    spdlog::shutdown();
    auto sync_logger = spdlog::basic_logger_mt("sync_perf", "logs/sync_perf.log", true);

    std::vector<long long> sync_latencies;
    for (int i = 0; i < iterations; i++) {
        auto t0 = std::chrono::steady_clock::now();
        sync_logger->info("Sync message #{}", i);
        auto t1 = std::chrono::steady_clock::now();
        sync_latencies.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }
    sync_logger->flush();

    // --- 异步日志测试 ---
    spdlog::drop("sync_perf");
    spdlog::init_thread_pool(65536, 1);
    auto async_logger = spdlog::basic_logger_mt<spdlog::async_factory>(
        "async_perf", "logs/async_perf.log", true);

    std::vector<long long> async_latencies;
    for (int i = 0; i < iterations; i++) {
        auto t0 = std::chrono::steady_clock::now();
        async_logger->info("Async message #{}", i);
        auto t1 = std::chrono::steady_clock::now();
        async_latencies.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }
    async_logger->flush();

    // --- 统计分析 ---
    auto calc_stats = [](const std::vector<long long> &v, const std::string &name) {
        long long total = std::accumulate(v.begin(), v.end(), 0LL);
        long long avg = total / (long long)v.size();
        long long min_val = *std::min_element(v.begin(), v.end());
        long long max_val = *std::max_element(v.begin(), v.end());

        // 计算 P99 (99th percentile)
        auto sorted = v;
        std::sort(sorted.begin(), sorted.end());
        long long p99 = sorted[sorted.size() * 99 / 100];

        std::cout << "  " << name << ":" << std::endl;
        std::cout << "    平均: " << avg / 1000.0 << " μs" << std::endl;
        std::cout << "    最小: " << min_val / 1000.0 << " μs" << std::endl;
        std::cout << "    P99:  " << p99 / 1000.0 << " μs" << std::endl;
        std::cout << "    最大: " << max_val / 1000.0 << " μs" << std::endl;
        return std::make_tuple(avg, max_val);
    };

    std::cout << std::endl;
    auto [sync_avg, sync_max] = calc_stats(sync_latencies, "同步日志(有 8KB 缓冲)");
    auto [async_avg, async_max] = calc_stats(async_latencies, "异步日志(纯队列入队)");

    std::cout << std::endl;
    std::cout << "  === 关键结论 ===" << std::endl;
    std::cout << "  同步平均更快: 因为 info() 只写内存缓冲区，没有真正的磁盘 I/O" << std::endl;
    std::cout << "  但同步最大延迟远高于异步: 当缓冲区满时触发磁盘写入，阻塞调用方" << std::endl;
    std::cout << std::endl;
    std::cout << "  最大延迟对比(影响控制循环实时性):" << std::endl;
    std::cout << "    同步最大: " << sync_max / 1000.0 << " μs";
    if (sync_max > 1000000) std::cout << " ← 磁盘 I/O 阻塞!超过 1ms!";
    std::cout << std::endl;
    std::cout << "    异步最大: " << async_max / 1000.0 << " μs (最多等入队)" << std::endl;
    std::cout << std::endl;
    std::cout << "  对于 1kHz 控制循环(周期 1000μs):" << std::endl;
    std::cout << "    同步: 平均快但偶发阻塞，可能导致控制抖动" << std::endl;
    std::cout << "    异步: 平均稍慢但延迟稳定，不会阻塞控制线程" << std::endl;
    std::cout << "    选异步的原因: 实时性看的是最坏情况，不是平均情况" << std::endl;

    spdlog::drop("async_perf");
}

int main() {
    try {
        std::cout << "=== 异步日志调优演示 ===" << std::endl;

        queue_size_demo();       // 演示不同队列大小的影响
        thread_count_demo();     // 演示多工作线程配置
        overflow_policy_demo();  // 演示队列溢出时的策略选择
        performance_comparison(); // 同步 vs 异步性能对比

        std::cout << "\n=== 异步日志调优演示完成 ===" << std::endl;
        spdlog::shutdown();  // 程序结束前必须 shutdown，刷新所有异步队列

    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
