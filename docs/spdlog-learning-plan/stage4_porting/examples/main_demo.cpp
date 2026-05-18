/**
 * 阶段 4: RoboLog 机械臂场景演示 (Robot Arm Scenario Demo)
 *
 * 本文件模拟一个六轴机械臂控制系统的日志场景:
 *   1. 系统启动   - 初始化控制器、驱动器、传感器
 *   2. 运动控制   - 执行运动指令、高频实时数据(trace级别)
 *   3. 错误处理   - 通信延迟、编码器异常、碰撞检测
 *   4. 系统关闭   - 保存状态、安全关闭
 *
 * RoboLog 是对 spdlog 的二次封装，专为机器人/嵌入式场景设计:
 *   - 隐藏 spdlog 的复杂配置，提供简洁的静态接口
 *   - 支持运行时配置(同步/异步、日志级别、文件轮转等)
 *   - 区分运行日志(全级别)和错误日志(仅 error 以上)
 *
 * 日志级别使用场景(本文件演示):
 *   - trace:    高频实时数据(关节角度、速度、力矩) — 生产环境关闭
 *   - debug:    调试信息 — 生产环境关闭
 *   - info:     正常运行信息(启动、运动指令、模式切换)
 *   - warn:     警告(接近限位、通信延迟)
 *   - error:    错误(编码器异常、通信中断)
 *   - critical: 严重错误(碰撞检测、紧急停机)
 */

#include "robo_log.h"
#include "robo_log_config.h"  // 预定义的配置模板(调试模式、生产模式等)

#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>  // 用于列出日志目录内容

/**
 * 模拟机械臂系统启动过程
 *
 * 典型的机器人系统启动序列:
 *   1. 初始化运动控制器(运动学求解器、轨迹规划器)
 *   2. 初始化各关节驱动器(伺服电机、减速器)
 *   3. 初始化传感器(力传感器、视觉系统、编码器)
 *   4. 自检通过后报告系统就绪
 *
 * 这些信息使用 info 级别，因为它们是正常的启动流程记录
 */
void simulate_robot_startup() {
    ROBOLOG_ERROR("系统启动完成"); 
    ROBOLOG_INFO("=== 机械臂系统启动 ===");
    ROBOLOG_INFO("初始化运动控制器...");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 模拟初始化耗时

    // 逐一初始化 6 个关节
    ROBOLOG_INFO("初始化关节驱动器...");
    ROBOLOG_INFO("  Joint 1: OK");
    ROBOLOG_INFO("  Joint 2: OK");
    ROBOLOG_INFO("  Joint 3: OK");
    ROBOLOG_INFO("  Joint 4: OK");
    ROBOLOG_INFO("  Joint 5: OK");
    ROBOLOG_INFO("  Joint 6: OK");

    // 初始化外设传感器
    ROBOLOG_INFO("初始化传感器...");
    ROBOLOG_INFO("  力传感器: OK");
    ROBOLOG_INFO("  视觉系统: OK");

    ROBOLOG_INFO("系统初始化完成");
}

/**
 * 模拟运动控制过程
 *
 * 展示不同日志级别在运动控制中的应用:
 *   - info:  运动指令下达、到达目标点、模式切换
 *   - trace: 高频实时数据(每毫秒的关节状态)
 *   - warn:  接近软限位(未超出但需要关注)
 *
 * trace 级别的高频数据:
 *   在 1kHz 控制循环中，每毫秒记录关节角度/速度/力矩
 *   生产环境通常关闭 trace，因为它会产生大量日志
 *   只在调试或数据采集时开启
 */
void simulate_motion_control() {
    ROBOLOG_INFO("=== 运动控制演示 ===");

    // 记录运动指令
    ROBOLOG_INFO("执行运动指令: 移动到目标点 A");
    ROBOLOG_INFO("  目标位置: x=0.5, y=0.3, z=0.4");

    // 模拟 10 个控制周期的实时数据(相当于 10ms@1kHz)
    // 使用 trace 级别: 生产环境默认关闭，调试时开启
    for (int i = 0; i < 10; i++) {
        double angle = 45.0 + i * 0.1;      // 关节角度(度)
        double velocity = 1.5 + i * 0.01;   // 角速度(rad/s)
        double torque = 10.0 + i * 0.5;     // 力矩(Nm)

        // spdlog 的格式化语法: {:.2f} 表示保留 2 位小数
        RoboLog::trace("Joint 1: angle={:.2f}°, velocity={:.2f} rad/s, torque={:.2f} Nm",
                       angle, velocity, torque);
    }

    ROBOLOG_INFO("到达目标点 A");

    // 警告: 关节接近软限位(未超出但需要关注)
    // warn 级别: 需要运维关注但不影响当前运行
    ROBOLOG_WARN("Joint 3 接近软限位: angle=89.5° (limit=90°)");

    // 模式切换记录
    ROBOLOG_INFO("切换到力控制模式");
    ROBOLOG_INFO("设置阻抗参数: K=100, D=10");
}

/**
 * 模拟错误处理过程
 *
 * 展示不同严重程度的错误日志:
 *   - warn:     通信延迟(可恢复，但需要监控)
 *   - error:    编码器异常(需要人工干预)
 *   - critical: 碰撞检测(需要紧急停机)
 *
 * 错误处理流程:
 *   1. 检测到异常 → warn/error
 *   2. 尝试恢复   → info
 *   3. 恢复成功   → info
 *   4. 严重故障   → critical + 紧急停机
 */
void simulate_error_handling() {
    ROBOLOG_INFO("=== 错误处理演示 ===");

    // 警告级别: 通信延迟但未中断
    ROBOLOG_WARN("CAN 总线通信延迟: 150ms (threshold=100ms)");

    // 错误级别: 编码器读数异常，需要处理
    ROBOLOG_ERROR("Joint 5 编码器读数异常");

    // 尝试恢复
    ROBOLOG_INFO("尝试重新初始化 Joint 5 编码器...");
    ROBOLOG_INFO("Joint 5 编码器恢复正常");

    // 严重错误: 碰撞检测，必须紧急停机
    // critical 级别: 触发安全保护，系统进入安全状态
    ROBOLOG_CRITICAL("碰撞检测触发！执行紧急停机");
    ROBOLOG_CRITICAL("所有关节已锁定");
}

/**
 * 模拟系统安全关闭
 *
 * 机器人系统的关闭顺序(与启动相反):
 *   1. 保存当前运动状态(下次启动可恢复)
 *   2. 关闭关节驱动器(伺服下电)
 *   3. 关闭通信接口(CAN、EtherCAT 等)
 *
 * 注意: shutdown 顺序很重要，先停上层再停下层
 */
void simulate_shutdown() {
    ROBOLOG_INFO("=== 机械臂系统关闭 ===");
    ROBOLOG_INFO("保存运动状态...");
    ROBOLOG_INFO("关闭关节驱动器...");
    ROBOLOG_INFO("关闭通信接口...");
    ROBOLOG_INFO("系统已安全关闭");
}

/**
 * 演示每日轮转 + 大小轮转功能
 *
 * 通过大量写入日志，触发大小轮转，展示:
 *   1. 文件名自动添加日期: robot_YYYY-MM-DD.log
 *   2. 超过大小限制后自动切分: robot_YYYY-MM-DD_1.log, _2.log, ...
 *   3. 最终列出 logs/ 目录下的所有日志文件
 *
 * 注意: 演示中使用较小的 max_file_size(1KB)来快速触发轮转。
 *       生产环境通常设置为 50MB+。
 */
void simulate_daily_rotation_demo() {
    ROBOLOG_INFO("=== 每日轮转演示 ===");
    ROBOLOG_INFO("将大量写入日志以触发大小轮转...");
    ROBOLOG_INFO("文件命名格式: robot_YYYY-MM-DD.log (主文件)");
    ROBOLOG_INFO("              robot_YYYY-MM-DD_N.log (轮转文件)");

    // 写入足够多的日志以触发大小轮转
    // 每条日志约 80-100 字节，1KB 阈值约 10-12 条即可触发一次轮转
    for (int i = 1; i <= 50; i++) {
        ROBOLOG_INFO_FMT("轮转测试日志 #{}: 这是一条用于触发大小轮转的测试消息，包含一些填充数据 "
                      "abcdefghij[{}]", i, i * 100);
    }

    ROBOLOG_INFO("每日轮转演示完成");
}

/**
 * 列出日志目录下的所有文件
 *
 * 使用 std::filesystem 遍历 logs/ 目录，展示轮转后的文件列表。
 */
void list_log_files() {
    std::cout << "\n--- logs/ 目录内容 ---" << std::endl;
    try {
        // 收集并排序日志文件
        std::vector<std::filesystem::path> log_files;
        for (const auto &entry : std::filesystem::directory_iterator("logs")) {
            if (entry.is_regular_file()) {
                log_files.push_back(entry.path());
            }
        }
        std::sort(log_files.begin(), log_files.end());

        for (const auto &file : log_files) {
            auto size = std::filesystem::file_size(file);
            std::cout << "  " << file.filename().string()
                      << " (" << size << " bytes)" << std::endl;
        }

        if (log_files.empty()) {
            std::cout << "  (空目录)" << std::endl;
        }
    } catch (const std::exception &ex) {
        std::cout << "  无法读取目录: " << ex.what() << std::endl;
    }
    std::cout << "---" << std::endl;
}

int main() {
    try {
        std::cout << "=== RoboLog 机械臂场景演示 ===" << std::endl;

        // ---- 第一部分: 基础功能演示(使用 debug 配置) ----
        std::cout << "\n[Part 1] 基础功能演示" << std::endl;
        RoboLog::init(debug_config());

        simulate_robot_startup();     // 系统启动
        simulate_motion_control();    // 运动控制
        simulate_error_handling();    // 错误处理
        simulate_shutdown();          // 安全关闭

        RoboLog::shutdown();
        list_log_files();

        // ---- 第二部分: 每日轮转演示(使用 daily 配置) ----
        std::cout << "\n[Part 2] 每日轮转演示" << std::endl;

        // 使用每日轮转配置:
        //   - daily_max_file_size = 1KB(故意设小以快速触发轮转)
        //   - daily_max_files = 5(同一天最多 5 个文件)
        auto daily_cfg = daily_config();
        daily_cfg.daily_max_file_size = 1024;  // 1KB，快速触发轮转
        daily_cfg.daily_max_files = 5;
        RoboLog::init(daily_cfg);

        simulate_daily_rotation_demo();

        RoboLog::shutdown();
        list_log_files();

        std::cout << "\n=== 演示完成 ===" << std::endl;
        std::cout << "检查 logs/ 目录查看轮转后的日志文件" << std::endl;

    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
