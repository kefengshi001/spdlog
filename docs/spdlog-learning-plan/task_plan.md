# spdlog 学习与移植计划

## 项目信息
- **学习对象**: spdlog (高性能 C++ 日志库)
- **目标项目**: 机械臂控制系统
- **分支**: v1.x
- **创建日期**: 2026-05-13
- **目标**: 从安装 → 熟悉 → 会用 → 移植，系统掌握 spdlog 并集成到机械臂控制系统

## 移植背景
机械臂控制系统需要可靠的日志系统来记录：
- 运动控制指令与执行状态
- 关节角度、速度、力矩等实时数据
- 异常报警与故障诊断信息
- 系统启动/关闭与配置变更日志

---

## 阶段 1: 安装与环境搭建
**状态**: 待开始
**目录**: [stage1_setup/](stage1_setup/)

### 目标
成功编译安装 spdlog，跑通第一个日志示例

### 任务
- [ ] 了解 spdlog 项目结构和依赖
- [ ] 使用 CMake 构建 spdlog（header-only 模式）
- [ ] 编写并运行第一个 Hello World 日志程序
- [ ] 验证安装：spdlog 1.17.0 工作正常

### 关键文件
- [design.md](stage1_setup/design.md) — 环境需求和构建配置
- [src/hello_world.cpp](stage1_setup/src/hello_world.cpp) — 最简日志示例
- [tests/test_install.cpp](stage1_setup/tests/test_install.cpp) — 安装验证

### 决策记录
- 选择 header-only 模式集成（减少部署复杂度）
- 使用 C++17 标准编译

---

## 阶段 2: 熟悉核心概念
**状态**: 待开始
**目录**: [stage2_core_concepts/](stage2_core_concepts/)

### 目标
理解 spdlog 的架构设计和核心 API

### 任务
- [ ] 学习 Logger、Sink、Formatter 三大核心概念
- [ ] 了解日志级别体系（trace/debug/info/warn/error/critical）
- [ ] 熟悉内置 Sink 类型（stdout、rotating_file、daily_file 等）
- [ ] 学习格式化模式（pattern 语法）
- [ ] 理解同步 vs 异步日志的区别

### 关键文件
- [design.md](stage2_core_concepts/design.md) — 架构和核心类关系
- [src/logger_demo.cpp](stage2_core_concepts/src/logger_demo.cpp) — Logger 演示
- [src/sink_demo.cpp](stage2_core_concepts/src/sink_demo.cpp) — Sink 演示
- [src/formatter_demo.cpp](stage2_core_concepts/src/formatter_demo.cpp) — Formatter 演示
- [src/async_demo.cpp](stage2_core_concepts/src/async_demo.cpp) — 异步日志演示

### 决策记录
_暂无_

---

## 阶段 3: 熟练使用
**状态**: 待开始
**目录**: [stage3_advanced/](stage3_advanced/)

### 目标
能够灵活运用 spdlog 满足各种日志需求

### 任务
- [ ] 掌握多 logger 管理（registry、default logger）
- [ ] 自定义 Sink 开发
- [ ] 自定义 Formatter 开发
- [ ] 异步日志配置与调优
- [ ] 日志文件轮转策略（rotating、daily、按大小）
- [ ] 多线程安全日志实践
- [ ] 条件日志与性能优化（lazy evaluation）
- [ ] 错误处理与回调机制

### 关键文件
- [design.md](stage3_advanced/design.md) — 自定义设计和性能优化
- [src/custom_sink.cpp](stage3_advanced/src/custom_sink.cpp) — 自定义 Sink
- [src/custom_formatter.cpp](stage3_advanced/src/custom_formatter.cpp) — 自定义 Formatter
- [src/async_tuning.cpp](stage3_advanced/src/async_tuning.cpp) — 异步调优
- [src/thread_safety.cpp](stage3_advanced/src/thread_safety.cpp) — 多线程安全

### 决策记录
_暂无_

---

## 阶段 4: 移植到机械臂控制系统
**状态**: 待开始
**目录**: [stage4_porting/](stage4_porting/)

### 目标
将 spdlog 集成到机械臂控制系统，满足实时性和可靠性需求

### 任务
- [ ] 确定集成方式（推荐 header-only，减少部署复杂度）
- [ ] 编写 CMakeLists.txt 集成脚本
- [ ] 适配目标平台（嵌入式 Linux / RTOS / 工控机）
- [ ] 封装机械臂专用日志接口层（如 RoboLog）
- [ ] 设计日志分级策略：
  - `trace`: 关节角度、速度等高频实时数据
  - `debug`: 控制算法中间变量
  - `info`: 运动指令、模式切换
  - `warn`: 软限位接近、通信延迟
  - `error`: 硬件故障、通信中断
  - `critical`: 紧急停机、碰撞检测
- [ ] 配置日志输出策略：
  - 控制台输出（调试模式）
  - 文件轮转（运行日志）
  - 独立错误日志文件
- [ ] 性能测试：确保日志不影响实时控制循环
- [ ] 编写团队使用文档和规范

### 关键文件
- [design.md](stage4_porting/design.md) — RoboLog 设计和日志策略
- [src/robo_log.h](stage4_porting/src/robo_log.h) — RoboLog 接口
- [src/robo_log.cpp](stage4_porting/src/robo_log.cpp) — RoboLog 实现
- [tests/test_performance.cpp](stage4_porting/tests/test_performance.cpp) — 性能测试

### 决策记录
_暂无_

---

## 整体进度
_暂无_
