// spdlog 阶段 2: Logger 概念演示
// 演示 Logger 的创建、管理和多 Sink 绑定
//
// Logger 是 spdlog 的核心组件，负责：
//   1. 接收用户的日志请求（info/warn/error 等）
//   2. 根据日志级别决定是否输出
//   3. 将格式化后的日志消息分发到一个或多个 Sink
//
// Logger 的核心设计：
//   - 每个 Logger 有一个唯一名称，可通过 Registry 全局管理
//   - 一个 Logger 可以绑定多个 Sink（一对多关系）
//   - Logger 自身有日志级别，低于该级别的日志会被过滤

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <iostream>
#include <memory>

// 演示基本的 Logger 创建和使用
// spdlog 提供工厂函数来快速创建常见的 Logger 配置
void basic_logger_demo() {
    std::cout << "\n=== 1. 基本 Logger 创建 ===" << std::endl;

    // stdout_color_mt 是一个工厂函数，创建一个带颜色输出的控制台 Logger
    // 参数 "console_logger" 是该 Logger 的唯一名称
    // _mt 后缀表示线程安全版本（multi-threaded），也有 _st（single-threaded）版本
    // 创建后会自动注册到 spdlog 的全局 Registry 中
    auto console_logger = spdlog::stdout_color_mt("console_logger");
    console_logger->info("Created console logger");

    // spdlog::get() 通过名称从全局 Registry 中查找已注册的 Logger
    // 返回 shared_ptr，如果名称不存在则返回 nullptr
    // 适用于在不同模块/文件中共享同一个 Logger
    auto retrieved = spdlog::get("console_logger");
    if (retrieved) {
        retrieved->info("Retrieved logger from registry");
    }

    // 从全局 Registry 中移除名为 "console_logger" 的 Logger
    // drop() 只是从注册表中移除，如果外部还有 shared_ptr 持有引用则不会立即销毁
    // 不 drop 也不会造成内存泄漏，程序结束时 spdlog::shutdown() 会清理所有 Logger
    spdlog::drop("console_logger");
}

// 演示多 Sink Logger 的创建和使用
// 一个 Logger 可以同时将日志输出到多个目标（Sink），这是 spdlog 最强大的特性之一
// 例如：同一条日志既输出到控制台用于实时监控，又写入文件用于持久化存储
void multi_sink_logger_demo() {
    std::cout << "\n=== 2. 多 Sink Logger ===" << std::endl;

    // 创建两个独立的 Sink 对象（使用 make_shared 管理生命周期）
    // stdout_color_sink_mt: 带 ANSI 颜色的控制台输出 Sink（_mt 线程安全）
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    // basic_file_sink_mt: 基础文件输出 Sink
    // 参数 1: 日志文件路径（相对于工作目录）
    // 参数 2: true 表示 truncate（截断模式，每次启动清空文件），false 表示 append（追加模式）
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/multi_sink.log", false);

    // 直接构造 Logger，传入名称和 Sink 列表（初始化列表）
    // 与工厂函数不同，这里手动构造，不会自动注册到全局 Registry
    spdlog::logger multi_logger("multi", {console_sink, file_sink});

    // 设置 Logger 的最低日志级别为 debug
    // 低于此级别的日志将被忽略（过滤顺序: trace < debug < info < warn < error < critical）
    multi_logger.set_level(spdlog::level::debug);

    // 同一条日志会同时输出到控制台和文件两个目标
    multi_logger.debug("This goes to both console and file");
    multi_logger.info("Multi-sink logger works");
}

// 演示 spdlog 的全局 Registry（注册表）管理功能
// Registry 是一个全局的 Logger 容器，用于管理所有命名的 Logger
// 通过 Registry 可以在任何地方通过名称获取 Logger，实现跨模块共享
void registry_demo() {
    std::cout << "\n=== 3. Registry 管理 ===" << std::endl;

    // 手动创建一个 Logger（未指定 Sink，使用默认的空 Logger）
    // make_shared 创建共享指针，确保 Logger 的生命周期由引用计数管理
    auto logger = std::make_shared<spdlog::logger>("my_logger");

    // 将 Logger 注册到全局 Registry，之后可通过 spdlog::get("my_logger") 获取
    // 注意: 如果已存在同名 Logger，register_logger 会抛出异常
    spdlog::register_logger(logger);

    // 通过名称从 Registry 中查找 Logger
    // spdlog::get() 返回 shared_ptr<logger>，名称不存在时返回 nullptr
    if (auto l = spdlog::get("my_logger")) {
        // 此时l 指向已注册的 Logger，但是他的sinks() 仍然是空的，因为我们创建时没有指定 Sink
        // 可以通过 l->sinks() 获取当前绑定的 Sink 列表，进行添加或修改
        l->sinks().push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        l->info("Got logger from registry");
    }
    else {
        std::cout << "Logger not found" << std::endl;
    }

    // apply_all() 遍历 Registry 中所有已注册的 Logger，对每个执行回调函数
    // 适用于调试时查看当前注册了哪些 Logger
    spdlog::apply_all([](std::shared_ptr<spdlog::logger> l) {
        std::cout << "  Registered: " << l->name() << std::endl;
    });

    // 从 Registry 中移除 "my_logger"
    // 注意: drop 只从注册表中移除，不影响外部持有的 shared_ptr
    spdlog::drop("my_logger");
}

int main() {
    try {
        std::cout << "=== Logger 概念演示 ===" << std::endl;

        // 依次运行三个演示函数
        basic_logger_demo();       // 1. 基本 Logger 创建与 Registry 查找
        multi_sink_logger_demo();  // 2. 多 Sink 绑定（同时输出到控制台和文件）
        registry_demo();           // 3. Registry 管理（注册、查找、遍历）

        std::cout << "\n=== Logger 演示完成 ===" << std::endl;

        // 关闭所有 Logger，刷新所有 Sink 的缓冲区，释放全局资源
        // 程序结束前必须调用，确保日志数据完整写出
        spdlog::shutdown();

    } catch (const spdlog::spdlog_ex &ex) {
        // spdlog::spdlog_ex 是 spdlog 的异常基类
        // 常见触发场景: 文件打开失败、Sink 初始化失败、重复注册同名 Logger 等
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
