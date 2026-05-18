// spdlog 阶段 2: 异步日志演示
// 演示同步 vs 异步日志的区别，通过计时对比两者的性能差异

// spdlog.h — spdlog 核心头文件，提供日志记录的主要 API
#include "spdlog/spdlog.h"
// stdout_color_sinks.h — 带颜色的控制台输出 sink（本文件未直接使用，但保留以备扩展）
#include "spdlog/sinks/stdout_color_sinks.h"
// basic_file_sink.h — 基础文件输出 sink，将日志写入文件
#include "spdlog/sinks/basic_file_sink.h"
// async.h — 异步日志支持，提供 async_factory 和线程池
#include "spdlog/async.h"

#include <iostream>
#include <chrono>

// ==================== 1. 同步日志演示 ====================
// 同步日志的特点：每次调用 info/warn/error 等方法时，
// 当前线程会阻塞，直到日志消息被实际写入 sink（如文件）后才返回。
// 优点：简单可靠，每条日志保证立即落盘
// 缺点：高频写入时会拖慢业务逻辑的执行速度
void sync_logger_demo() {
    std::cout << "\n=== 1. 同步日志 ===" << std::endl;

    // 记录开始时间，用于计算同步日志的总耗时
    auto start = std::chrono::steady_clock::now();

    // 创建一个线程安全的同步文件日志器
    // 参数说明：
    //   "sync"           — 日志器名称，用于后续通过 spdlog::get("sync") 获取
    //   "logs/sync.log"  — 日志文件路径
    //   true             — truncate，true 表示每次启动时清空文件重新写入
    auto sync_logger = spdlog::basic_logger_mt("sync", "logs/sync.log", true);

    // 写入 100 条日志，每条都会阻塞直到写入完成
    for (int i = 0; i < 100; i++) {
        sync_logger->info("Sync message #{}", i);
    }

    // flush() 强制将缓冲区中的日志立即写入 sink
    // 同步模式下通常不需要显式 flush（因为每次写入都已落盘），
    // 这里调用是为了确保计时准确
    sync_logger->flush();

    // 记录结束时间，计算并输出耗时
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Sync logging: " << duration.count() << " microseconds" << std::endl;

    // drop() 从 spdlog 的全局注册表中移除该日志器并释放资源
    // 如果不 drop，下次创建同名日志器会冲突
    spdlog::drop("sync");
}

// ==================== 2. 异步日志演示 ====================
// 异步日志的特点：日志消息先放入一个内存队列，由后台线程池负责
// 实际写入 sink。调用线程不会阻塞，可以立即继续执行业务逻辑。
// 优点：对业务逻辑的性能影响极小，适合高频日志场景
// 缺点：如果程序崩溃，队列中未写入的日志可能会丢失
void async_logger_demo() {
    std::cout << "\n=== 2. 异步日志 ===" << std::endl;

    // 初始化异步日志的全局线程池（整个进程只需初始化一次）
    // 参数说明：
    //   8192 — 消息队列大小，最多缓存 8192 条日志消息
    //          队列满时的行为由溢出策略决定（见下方 async_config_demo）
    //   1    — 工作线程数，1 个后台线程负责从队列取消息并写入 sink
    //          多线程可提高多 sink 场景下的写入吞吐量
    spdlog::init_thread_pool(8192, 1);

    auto start = std::chrono::steady_clock::now();

    // 创建异步文件日志器，模板参数 spdlog::async_factory 是关键区别
    // 它告诉 spdlog 使用异步模式创建日志器：
    //   1. 先创建一个后台 worker，绑定到全局线程池
    //   2. 日志消息先进入线程池的队列
    //   3. 后台线程从队列中取出消息，写入文件
    auto async_logger = spdlog::basic_logger_mt<spdlog::async_factory>(
        "async", "logs/async.log", true);

    // 写入 100 条日志，这些消息只是被放入队列，不会阻塞当前线程
    for (int i = 0; i < 100; i++) {
        async_logger->info("Async message #{}", i);
    }

    // flush() 在异步模式下会等待队列中的所有消息被处理完毕后才返回
    // 这确保计时结果包含所有消息的实际写入时间
    async_logger->flush();

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Async logging: " << duration.count() << " microseconds" << std::endl;
    spdlog::drop("async");
}

// ==================== 3. 异步日志配置说明 ====================
// 这个函数只是打印配置说明，不创建实际的日志器
void async_config_demo() {
    std::cout << "\n=== 3. 异步日志配置 ===" << std::endl;

    // 溢出策略（async_overflow_policy）决定了当日志队列满时如何处理新消息：
    //
    // block_retry（默认）：
    //   生产者线程阻塞等待，直到队列有空闲位置
    //   优点：不丢消息；缺点：可能阻塞业务线程
    //
    // discard_log：
    //   直接丢弃新消息
    //   优点：绝不阻塞业务线程；缺点：会丢失日志
    //
    // overrun_oldest：
    //   覆盖队列中最旧的未处理消息
    //   优点：不阻塞且保留最新日志；缺点：丢失旧日志

    // 队列满时的行为
    // spdlog::async_overflow_policy::block_retry  - 阻塞等待（默认）
    // spdlog::async_overflow_policy::discard_log  - 丢弃新消息
    // spdlog::async_overflow_policy::overrun_oldest - 覆盖最旧消息


    std::cout << "异步日志配置参数:" << std::endl;
    std::cout << "  队列大小: 影响缓冲能力，越大可缓存越多日志" << std::endl;
    std::cout << "  线程数: 影响并发写入能力，多 sink 时可增加" << std::endl;
    std::cout << "  溢出策略: 队列满时的行为（阻塞/丢弃/覆盖旧消息）" << std::endl;
}

// ==================== 主函数 ====================
int main() {
    try {
        std::cout << "=== 异步日志演示 ===" << std::endl;

        // 依次运行三个演示
        sync_logger_demo();     // 同步日志计时
        async_logger_demo();    // 异步日志计时
        async_config_demo();    // 打印配置说明

        std::cout << "\n=== 异步日志演示完成 ===" << std::endl;
        std::cout << "注意: 异步日志适合高频场景（如 1kHz 传感器数据）" << std::endl;
        std::cout << "同步日志适合关键信息（如控制指令、错误报警）" << std::endl;

        // shutdown() 是 spdlog 的全局清理函数，必须在程序结束前调用
        // 它会：
        //   1. 刷新所有日志器（确保队列中的消息全部写入）
        //   2. 关闭线程池（等待后台线程结束）
        //   3. 释放所有注册的日志器
        // 不调用 shutdown() 可能导致程序退出时丢失异步队列中的日志
        spdlog::shutdown();

    } catch (const spdlog::spdlog_ex &ex) {
        // spdlog_ex 是 spdlog 的异常基类，捕获所有 spdlog 相关错误
        // 常见错误：文件无法创建、权限不足等
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
