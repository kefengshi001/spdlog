# 研究与发现

## 项目信息
- **学习对象**: spdlog
- **目标项目**: 机械臂控制系统
- **分支**: v1.x
- **创建日期**: 2026-05-13

---

## spdlog 项目概览

### 基本信息
- **作者**: Gabi Melman
- **许可证**: MIT
- **语言**: C++11/14/17
- **特点**: 高性能、header-only、跨平台

### 核心架构
- **Logger**: 日志记录器，核心入口
- **Sink**: 日志输出目标（控制台、文件、网络等）
- **Formatter**: 日志格式化（pattern 语法）
- **Registry**: 全局 logger 管理器

### 内置 Sink 类型（共 35 种）
**控制台输出:**
- `stdout_color_sink` - 彩色控制台输出
- `stdout_sink` - 标准控制台输出
- `stderr_color_sink` - 彩色标准错误输出

**文件输出:**
- `basic_file_sink` - 基础文件输出
- `rotating_file_sink` - 按大小轮转文件
- `daily_file_sink` - 按日期轮转文件
- `hourly_file_sink` - 按小时轮转文件

**系统日志:**
- `syslog_sink` - 系统日志（Linux/macOS）
- `systemd_sink` - systemd 日志
- `android_sink` - Android 日志

**网络输出:**
- `tcp_sink` - TCP 网络输出
- `udp_sink` - UDP 网络输出
- `kafka_sink` - Kafka 消息队列
- `mongo_sink` - MongoDB 数据库

**特殊用途:**
- `null_sink` - 空输出（丢弃日志）
- `callback_sink` - 回调函数输出
- `ringbuffer_sink` - 环形缓冲区
- `dist_sink` - 分发到多个 Sink

### 异步日志
- 使用独立线程处理日志写入
- 支持队列大小配置
- 支持队列满时的丢弃策略

---

## 机械臂控制系统日志需求分析

### 日志分类与级别映射
| 日志类型 | 级别 | 频率 | 示例 |
|---------|------|------|------|
| 实时数据 | trace | 高频(1kHz+) | 关节角度、速度、力矩 |
| 算法调试 | debug | 中频 | PID 参数、轨迹规划 |
| 运行事件 | info | 低频 | 指令下发、模式切换 |
| 警告信息 | warn | 低频 | 软限位接近、通信延迟 |
| 故障报警 | error | 极低频 | 硬件故障、通信中断 |
| 紧急事件 | critical | 极低频 | 碰撞检测、急停触发 |

### 性能要求
- 实时控制循环周期: 1ms (1kHz)
- 日志写入不能阻塞控制线程
- 建议: 高频数据使用异步日志 + 独立 Sink

---

## 发现 1: spdlog 版本与编译环境
**日期**: 2026-05-13
**来源**: 阶段 1 安装验证

### 描述
- spdlog 版本: 1.17.0
- 编译器: GCC 11.4.0
- C++ 标准: C++17
- 构建模式: header-only（推荐）

### 影响
- header-only 模式无需编译为库，减少部署复杂度
- C++17 标准支持所有 spdlog 功能

### 建议
- 机械臂控制系统推荐使用 header-only 模式
- 使用 C++17 或更高标准以获得最佳兼容性

---

## 发现 2: spdlog 项目结构
**日期**: 2026-05-13
**来源**: 项目目录分析

### 描述
```
spdlog/
├── include/spdlog/          # 核心头文件
│   ├── spdlog.h             # 主入口
│   ├── logger.h             # Logger 类
│   ├── sinks/               # 35 种 Sink 实现
│   ├── fmt/                 # 格式化库
│   └── details/             # 内部实现
├── src/                     # 编译为库时的源文件
├── example/                 # 官方示例
├── tests/                   # 测试代码
└── bench/                   # 性能测试
```

### 影响
- header-only 模式只需 include 目录
- example.cpp 包含所有功能示例，可作为学习参考

### 建议
- 学习时参考 example/example.cpp
- 移植时只需复制 include/spdlog/ 目录

---

## 待解决问题
- [ ] _暂无_

## 已解决问题
- [ ] _暂无_
