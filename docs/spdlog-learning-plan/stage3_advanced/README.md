# 阶段 3: 高级用法

## 目标
掌握自定义 Sink/Formatter、异步调优、多线程安全等高级功能

## 任务清单
- [ ] 自定义 Sink 开发
- [ ] 自定义 Formatter 开发
- [ ] 异步日志配置与调优
- [ ] 日志文件轮转策略
- [ ] 多线程安全日志实践
- [ ] 条件日志与性能优化
- [ ] 错误处理与回调机制

## 文件说明
| 文件 | 用途 |
|------|------|
| `design.md` | 自定义 Sink/Formatter 设计、性能优化策略 |
| `src/custom_sink.cpp` | 自定义 Sink 开发示例 |
| `src/custom_formatter.cpp` | 自定义 Formatter 开发示例 |
| `src/async_tuning.cpp` | 异步日志调优 |
| `src/log_rotation.cpp` | 日志轮转策略 |
| `src/thread_safety.cpp` | 多线程安全实践 |
| `tests/test_advanced.cpp` | 高级功能测试 |
| `CMakeLists.txt` | 构建脚本 |
| `summary.md` | 最佳实践总结 |

## 完成标准
1. 能开发自定义 Sink 和 Formatter
2. 能配置异步日志参数
3. 能设计日志轮转策略
4. 能在多线程环境下安全使用日志
5. 所有测试通过

## 构建与运行
```bash
cd stage3_advanced
mkdir build && cd build
cmake ..
make
./custom_sink
./custom_formatter
./async_tuning
./log_rotation
./thread_safety
ctest
```
