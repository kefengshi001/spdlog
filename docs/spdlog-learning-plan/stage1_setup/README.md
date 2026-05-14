# 阶段 1: 安装与环境搭建

## 目标
成功编译安装 spdlog，跑通第一个日志示例

## 前置依赖
- GCC 11+ 或 Clang 14+
- CMake 3.10+
- C++17 标准

## 任务清单
- [ ] 了解 spdlog 项目结构和依赖
- [ ] 使用 CMake 构建 spdlog（header-only 模式）
- [ ] 编写并运行第一个 Hello World 日志程序
- [ ] 验证安装：spdlog 1.17.0 工作正常

## 文件说明
| 文件 | 用途 |
|------|------|
| `design.md` | 环境需求、构建配置说明 |
| `src/hello_world.cpp` | 最简日志示例 |
| `src/multi_sink_demo.cpp` | 控制台+文件双输出示例 |
| `tests/test_install.cpp` | 验证安装功能 |
| `CMakeLists.txt` | 构建脚本 |
| `summary.md` | 阶段完成总结 |

## 完成标准
1. `hello_world.cpp` 能编译运行，控制台输出日志
2. `multi_sink_demo.cpp` 能同时输出到控制台和文件
3. `test_install.cpp` 全部测试通过
4. 环境信息记录在 `summary.md` 中

## 构建与运行
```bash
cd stage1_setup
mkdir build && cd build
cmake ..
make
./hello_world
./multi_sink_demo
ctest
```
