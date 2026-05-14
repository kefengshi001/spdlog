// spdlog 阶段 1: 多 Sink 示例
// 验证同时输出到控制台和文件

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <memory>

int main() {
    try {
        // 创建控制台 Sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);

        // 创建文件 Sink
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/multi_sink.log", true);
        file_sink->set_level(spdlog::level::debug);

        // 创建 Logger 使用多个 Sink
        spdlog::logger logger("multi_sink", {console_sink, file_sink});
        logger.set_level(spdlog::level::debug);

        logger.debug("This debug message goes to file only");
        logger.info("This info message goes to console and file");
        logger.warn("This warning goes to console and file");
        logger.error("This error goes to console and file");

        spdlog::info("Multi sink demo completed. Check logs/multi_sink.log");

    } catch (const spdlog::spdlog_ex &ex) {
        spdlog::error("spdlog error: {}", ex.what());
        return 1;
    }

    return 0;
}
