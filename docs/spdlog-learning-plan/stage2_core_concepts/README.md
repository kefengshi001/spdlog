# 阶段 2: 熟悉核心概念

## 目标
理解 spdlog 的架构设计和核心 API

## 学习路径
1. **Logger** — 日志记录器，核心入口
2. **Sink** — 日志输出目标
3. **Formatter** — 日志格式化
4. **日志级别** — trace/debug/info/warn/error/critical
5. **同步 vs 异步** — 两种日志模式

## 任务清单
- [ ] 学习 Logger、Sink、Formatter 三大核心概念
- [ ] 了解日志级别体系
- [ ] 熟悉内置 Sink 类型
- [ ] 学习格式化模式（pattern 语法）
- [ ] 理解同步 vs 异步日志的区别

## 文件说明
| 文件 | 用途 |
|------|------|
| `design.md` | spdlog 架构、核心类关系、数据流 |
| `src/logger_demo.cpp` | Logger 创建与管理 |
| `src/sink_demo.cpp` | 各类 Sink 使用 |
| `src/formatter_demo.cpp` | pattern 语法详解 |
| `src/async_demo.cpp` | 异步日志配置 |
| `tests/test_core_concepts.cpp` | 核心概念测试 |
| `CMakeLists.txt` | 构建脚本 |
| `summary.md` | 核心概念总结、API 速查表 |

## 完成标准
1. 能解释 Logger/Sink/Formatter 的职责和关系
2. 能使用各类内置 Sink
3. 能自定义日志格式
4. 能配置同步/异步日志
5. 所有测试通过

## 构建与运行
```bash
cd stage2_core_concepts
mkdir build && cd build
cmake ..
make
./logger_demo
./sink_demo
./formatter_demo
./async_demo
ctest
```
