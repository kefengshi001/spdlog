# spdlog 学习计划 — 分阶段文件结构设计

## 项目信息
- **学习对象**: spdlog v1.17.0 (高性能 C++ 日志库)
- **目标项目**: 机械臂控制系统
- **分支**: v1.x
- **创建日期**: 2026-05-13

## 目标

将现有的 spdlog 学习计划拆分为 4 个独立阶段文件夹，每个阶段包含完整的开发文档、示例代码和测试代码。

## 设计决策

### 文件夹组织方式
采用扁平结构（方案 A）：每个阶段文件夹内直接放 README.md、src/、tests/，不额外嵌套 docs/code/tests 子目录。

### 阶段划分

#### 阶段 1: stage1_setup/ — 安装与环境搭建
- **目标**: 成功编译安装 spdlog，跑通第一个日志示例
- **文件**:
  - `README.md` — 阶段目标、任务清单、前置依赖、完成标准
  - `design.md` — 环境需求（GCC/CMake/C++17）、header-only vs 编译库对比、构建配置说明
  - `src/hello_world.cpp` — 最简日志示例
  - `src/multi_sink_demo.cpp` — 控制台+文件双输出示例
  - `tests/test_install.cpp` — 验证安装功能
  - `CMakeLists.txt` — 构建脚本
  - `summary.md` — 阶段总结

#### 阶段 2: stage2_core_concepts/ — 核心概念
- **目标**: 理解 Logger、Sink、Formatter 三大核心概念
- **文件**:
  - `README.md` — 核心概念详解、学习路径
  - `design.md` — spdlog 架构图、核心类关系、数据流
  - `src/logger_demo.cpp` — Logger 创建与管理
  - `src/sink_demo.cpp` — 各类 Sink 使用
  - `src/formatter_demo.cpp` — pattern 语法详解
  - `src/async_demo.cpp` — 异步日志配置
  - `tests/test_core_concepts.cpp` — 核心概念测试
  - `CMakeLists.txt` — 构建脚本
  - `summary.md` — 核心概念总结、API 速查表

#### 阶段 3: stage3_advanced/ — 高级用法
- **目标**: 掌握自定义 Sink/Formatter、异步调优、多线程安全
- **文件**:
  - `README.md` — 高级功能清单
  - `design.md` — 自定义 Sink/Formatter 设计、性能优化策略
  - `src/custom_sink.cpp` — 自定义 Sink 开发
  - `src/custom_formatter.cpp` — 自定义 Formatter 开发
  - `src/async_tuning.cpp` — 异步日志调优
  - `src/log_rotation.cpp` — 日志轮转策略
  - `src/thread_safety.cpp` — 多线程安全实践
  - `tests/test_advanced.cpp` — 高级功能测试
  - `CMakeLists.txt` — 构建脚本
  - `summary.md` — 最佳实践总结

#### 阶段 4: stage4_porting/ — 移植到机械臂控制系统
- **目标**: 将 spdlog 集成到机械臂控制系统，满足实时性和可靠性需求
- **文件**:
  - `README.md` — 移植目标、集成方式、团队使用指南
  - `design.md` — RoboLog 封装层设计、日志分级策略、输出策略
  - `src/robo_log.h` — RoboLog 头文件
  - `src/robo_log.cpp` — RoboLog 实现
  - `src/robo_log_config.h` — 配置文件
  - `src/main_demo.cpp` — 机械臂场景演示
  - `tests/test_robo_log.cpp` — 单元测试
  - `tests/test_performance.cpp` — 性能测试
  - `CMakeLists.txt` — 构建脚本
  - `summary.md` — 移植总结、部署指南

### 现有文件处理
- `task_plan.md` — 保留，作为总计划入口
- `findings.md` — 保留，作为研究记录
- `stage2_core_concepts.cpp` — 移动到 `stage2_core_concepts/src/` 并拆分为多个文件
- `README.md` — 更新，添加各阶段文件夹链接

### 构建系统
每个阶段使用独立的 CMakeLists.txt，支持：
- 单独构建该阶段的示例和测试
- 自动发现并链接 spdlog（header-only 模式）
- 使用 C++17 标准

## 范围
- 仅创建文件夹结构和文档框架
- 示例代码和测试代码在后续阶段逐步填充
- 不修改 spdlog 源码
