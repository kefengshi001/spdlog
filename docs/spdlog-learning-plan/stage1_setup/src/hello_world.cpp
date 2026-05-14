// spdlog 阶段 1: Hello World 示例
// 验证 spdlog 基本功能：日志输出到控制台

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

int main() {
    // 使用默认 logger 输出到控制台
    spdlog::info("Hello, spdlog!");
    spdlog::warn("This is a warning message");
    spdlog::error("This is an error message");

    // 使用格式化输出
    spdlog::info("spdlog version: {}.{}", 1, 17);

    return 0;
}
