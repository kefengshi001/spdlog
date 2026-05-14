// RoboLog 机械臂场景演示

#include "robo_log.h"
#include "robo_log_config.h"

#include <iostream>
#include <thread>
#include <chrono>

void simulate_robot_startup() {
    RoboLog::info("=== 机械臂系统启动 ===");
    RoboLog::info("初始化运动控制器...");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    RoboLog::info("初始化关节驱动器...");
    RoboLog::info("  Joint 1: OK");
    RoboLog::info("  Joint 2: OK");
    RoboLog::info("  Joint 3: OK");
    RoboLog::info("  Joint 4: OK");
    RoboLog::info("  Joint 5: OK");
    RoboLog::info("  Joint 6: OK");

    RoboLog::info("初始化传感器...");
    RoboLog::info("  力传感器: OK");
    RoboLog::info("  视觉系统: OK");

    RoboLog::info("系统初始化完成");
}

void simulate_motion_control() {
    RoboLog::info("=== 运动控制演示 ===");

    // 模拟运动指令
    RoboLog::info("执行运动指令: 移动到目标点 A");
    RoboLog::info("  目标位置: x=0.5, y=0.3, z=0.4");

    // 模拟高频实时数据（trace 级别）
    for (int i = 0; i < 10; i++) {
        double angle = 45.0 + i * 0.1;
        double velocity = 1.5 + i * 0.01;
        double torque = 10.0 + i * 0.5;

        RoboLog::trace("Joint 1: angle={:.2f}°, velocity={:.2f} rad/s, torque={:.2f} Nm",
                       angle, velocity, torque);
    }

    RoboLog::info("到达目标点 A");

    // 模拟警告
    RoboLog::warn("Joint 3 接近软限位: angle=89.5° (limit=90°)");

    // 模拟模式切换
    RoboLog::info("切换到力控制模式");
    RoboLog::info("设置阻抗参数: K=100, D=10");
}

void simulate_error_handling() {
    RoboLog::info("=== 错误处理演示 ===");

    // 模拟通信延迟警告
    RoboLog::warn("CAN 总线通信延迟: 150ms (threshold=100ms)");

    // 模拟错误
    RoboLog::error("Joint 5 编码器读数异常");

    // 模拟恢复
    RoboLog::info("尝试重新初始化 Joint 5 编码器...");
    RoboLog::info("Joint 5 编码器恢复正常");

    // 模拟严重错误
    RoboLog::critical("碰撞检测触发！执行紧急停机");
    RoboLog::critical("所有关节已锁定");
}

void simulate_shutdown() {
    RoboLog::info("=== 机械臂系统关闭 ===");
    RoboLog::info("保存运动状态...");
    RoboLog::info("关闭关节驱动器...");
    RoboLog::info("关闭通信接口...");
    RoboLog::info("系统已安全关闭");
}

int main() {
    try {
        std::cout << "=== RoboLog 机械臂场景演示 ===" << std::endl;

        // 使用调试模式配置
        RoboLog::init(debug_config());

        simulate_robot_startup();
        simulate_motion_control();
        simulate_error_handling();
        simulate_shutdown();

        RoboLog::shutdown();

        std::cout << "\n=== 演示完成 ===" << std::endl;
        std::cout << "检查 logs/ 目录查看日志文件" << std::endl;

    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
