// spdlog 阶段 1: 多 Sink 示例
// 验证同时输出到控制台和文件

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <memory>

int main() {
    try {

        // set_level 的核心作用是设置日志级别过滤器，只有级别 >= 设定值的日志才会被输出。
        // 日志级别从低到高: trace(0) < debug(1) < info(2) < warn(3) < error(4) < critical(5) < off(6)


        // 创建控制台 Sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        // 确定控制台输出日志级别
        console_sink->set_level(spdlog::level::info);
        // 打印当前console_sink的日志级别
        // spdlog::level::to_short_c_str() 将枚举转成短字符串
        spdlog::info("Console sink log level: {}", spdlog::level::to_short_c_str(console_sink->level()));

        // 创建文件 Sink
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/multi_sink.log", true);
        // 设置文件输出日志级别
        file_sink->set_level(spdlog::level::warn);
        // 打印当前file_sink的日志级别
        spdlog::info("File sink log level: {}", spdlog::level::to_short_c_str(file_sink->level()));

        // 创建 Logger 使用多个 Sink
        spdlog::logger logger("multi_sink", {console_sink, file_sink});
        // 设置整合的日志级别，第一到门槛，只有级别 >= debug 的日志才会被 logger 处理，然后再由各个 sink 根据自己的级别过滤输出
        logger.set_level(spdlog::level::debug);
        // 打印当前logger的日志级别
        spdlog::info("Logger log level: {}", spdlog::level::to_short_c_str(logger.level()));

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

