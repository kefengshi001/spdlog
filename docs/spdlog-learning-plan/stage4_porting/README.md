# 阶段 4: 移植到机械臂控制系统

## 目标
将 spdlog 集成到机械臂控制系统，满足实时性和可靠性需求

## 移植背景
机械臂控制系统需要可靠的日志系统来记录：
- 运动控制指令与执行状态
- 关节角度、速度、力矩等实时数据
- 异常报警与故障诊断信息
- 系统启动/关闭与配置变更日志

## 任务清单
- [ ] 确定集成方式（header-only）
- [ ] 编写 CMakeLists.txt 集成脚本
- [ ] 封装机械臂专用日志接口层（RoboLog）
- [ ] 设计日志分级策略
- [ ] 配置日志输出策略
- [ ] 性能测试：确保日志不影响实时控制循环
- [ ] 编写团队使用文档和规范

## 文件说明
| 文件 | 用途 |
|------|------|
| `README.md` | 移植目标、集成方式、团队使用指南 |
| `design.md` | RoboLog 封装层设计、日志分级策略、输出策略 |
| `src/robo_log.h` | RoboLog 头文件（机械臂专用日志接口） |
| `src/robo_log.cpp` | RoboLog 实现 |
| `src/robo_log_config.h` | 配置文件 |
| `src/main_demo.cpp` | 机械臂场景演示 |
| `tests/test_robo_log.cpp` | 单元测试 |
| `tests/test_performance.cpp` | 性能测试 |
| `CMakeLists.txt` | 构建脚本 |
| `summary.md` | 移植总结、部署指南 |

## 日志分级策略
| 日志类型 | 级别 | 频率 | 示例 |
|---------|------|------|------|
| 实时数据 | trace | 高频(1kHz+) | 关节角度、速度、力矩 |
| 算法调试 | debug | 中频 | PID 参数、轨迹规划 |
| 运行事件 | info | 低频 | 指令下发、模式切换 |
| 警告信息 | warn | 低频 | 软限位接近、通信延迟 |
| 故障报警 | error | 极低频 | 硬件故障、通信中断 |
| 紧急事件 | critical | 极低频 | 碰撞检测、急停触发 |

## 输出策略
- 控制台输出（调试模式）
- 文件轮转（运行日志）
- 独立错误日志文件

## 完成标准
1. RoboLog 接口封装完成
2. 日志分级策略实现
3. 性能测试通过（日志不阻塞 1kHz 控制循环）
4. 团队使用文档完成
5. 所有测试通过

## 构建与运行
```bash
cd stage4_porting
mkdir build && cd build
cmake ..
make
./main_demo
./test_robo_log
./test_performance
ctest
```
