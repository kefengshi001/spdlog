// spdlog 阶段 2: Logger 概念演示
// 演示 Logger 的创建、管理和多 Sink 绑定

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <iostream>
#include <memory>

void basic_logger_demo() {
    std::cout << "\n=== 1. 基本 Logger 创建 ===" << std::endl;

    // 使用工厂函数创建 Logger
    auto console_logger = spdlog::stdout_color_mt("console_logger");
    console_logger->info("Created console logger");

    // 使用 Registry 管理 Logger
    auto retrieved = spdlog::get("console_logger");
    if (retrieved) {
        retrieved->info("Retrieved logger from registry");
    }

    // 清理
    spdlog::drop("console_logger");
}

void multi_sink_logger_demo() {
    std::cout << "\n=== 2. 多 Sink Logger ===" << std::endl;

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/multi_sink.log", true);

    // Logger 可以绑定多个 Sink
    spdlog::logger multi_logger("multi", {console_sink, file_sink});
    multi_logger.set_level(spdlog::level::debug);

    multi_logger.debug("This goes to both console and file");
    multi_logger.info("Multi-sink logger works");
}

void registry_demo() {
    std::cout << "\n=== 3. Registry 管理 ===" << std::endl;

    // 注册 Logger
    auto logger = std::make_shared<spdlog::logger>("my_logger");
    spdlog::register_logger(logger);

    // 通过名称获取
    if (auto l = spdlog::get("my_logger")) {
        l->info("Got logger from registry");
    }

    // 列出所有注册的 Logger
    spdlog::apply_all([](std::shared_ptr<spdlog::logger> l) {
        std::cout << "  Registered: " << l->name() << std::endl;
    });

    // 清理
    spdlog::drop("my_logger");
}

int main() {
    try {
        std::cout << "=== Logger 概念演示 ===" << std::endl;

        basic_logger_demo();
        multi_sink_logger_demo();
        registry_demo();

        std::cout << "\n=== Logger 演示完成 ===" << std::endl;
        spdlog::shutdown();

    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
