/**
 * 示例：逆运动学求解器（使用 RoboLog 记录日志）
 *
 * 演示如何在自己的算法代码中使用 RoboLog：
 *   1. 包含 robo_log.h 即可，不需要关心 spdlog 细节
 *   2. 用 ROBOLOG_xxx 宏记录不同级别的日志
 *   3. 用 ROBOLOG_xxx_FMT 宏做格式化输出
 */

#include "robo_log.h"

#include <cmath>
#include <vector>
#include <iostream>

// 模拟的关节结构
struct JointState {
    double angle;     // 关节角度（度）
    double velocity;  // 角速度（rad/s）
    double torque;    // 力矩（Nm）
};

/**
 * 简化的逆运动学求解器
 */
class IKSolver {
public:
    IKSolver() {
        ROBOLOG_INFO("IKSolver 初始化完成");
    }

    /**
     * 求解逆运动学
     * @param target_x 目标 x 坐标
     * @param target_y 目标 y 坐标
     * @param target_z 目标 z 坐标
     * @return 求解成功返回 6 个关节角度
     */
    std::vector<double> solve(double target_x, double target_y, double target_z) {
        ROBOLOG_INFO_FMT("开始求解 IK: 目标位置 ({:.3f}, {:.3f}, {:.3f})",
                         target_x, target_y, target_z);

        // 模拟迭代求解过程
        const int max_iterations = 50;
        double residual = 1.0;

        for (int i = 0; i < max_iterations; i++) {
            residual = 1.0 / (i + 1) * std::sqrt(target_x * target_x + target_y * target_y);

            // trace 级别：每步迭代的详细数据（生产环境关闭）
            ROBOLOG_TRACE_FMT("迭代 {}: 残差 = {:.6f}", i, residual);

            if (residual < 1e-4) {
                ROBOLOG_INFO_FMT("IK 求解成功，迭代 {} 次，残差 {:.6f}", i + 1, residual);
                return {45.0, -30.0, 60.0, 0.0, 45.0, 0.0};  // 模拟结果
            }
        }

        // 超过最大迭代次数 → 警告
        ROBOLOG_WARN_FMT("IK 求解未收敛，迭代 {} 次，残差 {:.6f}", max_iterations, residual);
        return {};
    }
};

/**
 * 模拟机械臂运动控制
 */
void run_motion_loop() {
    ROBOLOG_INFO("进入运动控制循环");

    IKSolver solver;

    // 模拟几个运动指令
    struct MotionCmd {
        double x, y, z;
        const char *desc;
    };

    std::vector<MotionCmd> commands = {
        {0.5, 0.3, 0.4, "抓取位置"},
        {0.2, 0.5, 0.3, "放置位置"},
        {0.0, 0.0, 0.0, " home 位置"},
    };

    for (const auto &cmd : commands) {
        ROBOLOG_INFO_FMT("执行运动指令: {} -> ({:.2f}, {:.2f}, {:.2f})",
                         cmd.desc, cmd.x, cmd.y, cmd.z);

        auto joints = solver.solve(cmd.x, cmd.y, cmd.z);

        if (joints.empty()) {
            ROBOLOG_ERROR_FMT("运动指令失败: {}", cmd.desc);
            continue;
        }

        // trace：记录关节状态
        for (size_t i = 0; i < joints.size(); i++) {
            ROBOLOG_TRACE_FMT("Joint {}: angle = {:.2f}°", i + 1, joints[i]);
        }

        ROBOLOG_INFO_FMT("运动指令完成: {}", cmd.desc);
    }
}

int main() {
    // 方式 1：使用预设配置（最简单）
    // RoboLog::init(debug_config());

    // 方式 2：自定义配置
    RoboLogConfig config;
    config.level = spdlog::level::trace;           // 记录所有级别（含 trace）
    config.console_enabled = true;                 // 控制台输出
    config.daily_rotating_enabled = true;          // 每日轮转 + 大小轮转
    config.daily_max_file_size = 10 * 1024 * 1024; // 10MB
    config.daily_max_files = 5;
    config.error_file_enabled = true;              // 错误日志单独文件
    config.async_enabled = false;                  // 同步模式（调试用）
    RoboLog::init(config);

    ROBOLOG_INFO("=== 逆运动学算法程序启动 ===");

    run_motion_loop();

    ROBOLOG_INFO("=== 程序正常退出 ===");
    RoboLog::shutdown();

    return 0;
}
