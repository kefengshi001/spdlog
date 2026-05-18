# RoboLog 移植指南

将 RoboLog 日志系统移植到你自己的算法项目中。

## 需要复制的文件

只需复制整个 `log/` 文件夹（独立模块，自带 CMakeLists.txt）：

```
your_project/
├── CMakeLists.txt              # 你的项目顶层 CMakeLists
├── log/                        # 整个文件夹复制过来
│   ├── CMakeLists.txt          #   robo_log 静态库（无需修改）
│   ├── include/
│   │   ├── daily_rotating_file_sink.h
│   │   ├── robo_log.h
│   │   └── robo_log_config.h   #   预设配置模板（可选）
│   └── src/
│       └── robo_log.cpp
└── src/
    └── your_algorithm.cpp      # 你的算法代码
```

`log/` 内部结构：

| 文件 | 说明 |
|------|------|
| `CMakeLists.txt` | 定义 `robo_log` 静态库，链接 spdlog 和 Threads |
| `include/daily_rotating_file_sink.h` | 自定义 sink，实现每日+大小组合轮转 |
| `include/robo_log.h` | 接口定义、`RoboLogConfig` 结构体、日志宏 |
| `include/robo_log_config.h` | 预设配置模板（debug/production/daily 等） |
| `src/robo_log.cpp` | `RoboLog::init()` 等方法的实现 |

## 前置条件

- **spdlog** 已安装（`find_package(spdlog REQUIRED)` 能找到）
- **C++17** 或更高
- **CMake 3.10+**

## CMakeLists.txt 配置

在你项目的顶层 `CMakeLists.txt` 中添加两行即可：

```cmake
cmake_minimum_required(VERSION 3.10)
project(your_project CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(spdlog REQUIRED)
find_package(Threads REQUIRED)

# ---- 引入日志库 ----
add_subdirectory(log)

# ---- 你的算法目标 ----
add_executable(your_app
    src/your_algorithm.cpp
)
target_link_libraries(your_app PRIVATE robo_log)
```

关键点：
- `add_subdirectory(log)` — 引入日志库模块，自动定义 `robo_log` 目标
- `target_link_libraries(your_app PRIVATE robo_log)` — 链接日志库
- 不需要手动写 `spdlog::spdlog` 或 `Threads::Threads`，`robo_log` 用 `PUBLIC` 已自动传递

## 使用方式

### 1. 初始化（main 开头）

```cpp
#include "robo_log.h"
#include "robo_log_config.h"  // 如果复制了预设配置

int main() {
    // 方式 A: 使用预设配置
    RoboLog::init(production_config());

    // 方式 B: 自定义配置
    RoboLogConfig config;
    config.level = spdlog::level::info;
    config.console_enabled = false;
    config.file_path = "logs/algorithm.log";
    config.error_file_path = "logs/algorithm_error.log";
    config.daily_rotating_enabled = true;
    config.daily_max_file_size = 50 * 1024 * 1024;  // 50MB
    config.daily_max_files = 10;
    config.async_enabled = true;
    RoboLog::init(config);

    // ... 你的算法代码 ...

    RoboLog::shutdown();
    return 0;
}
```

### 2. 记录日志（任意位置）

```cpp
#include "robo_log.h"

// 简单字符串
ROBOLOG_INFO("算法初始化完成");
ROBOLOG_ERROR("逆运动学求解失败");

// 格式化输出
ROBOLOG_INFO_FMT("关节角度: {:.2f}°", angle);
ROBOLOG_ERROR_FMT("迭代 {} 次未收敛，残差: {:.6f}", iter, residual);
```

### 3. 日志级别选择

| 宏 | 级别 | 典型用途 |
|-----|------|----------|
| `ROBOLOG_TRACE_FMT` | trace | 高频实时数据（生产环境关闭） |
| `ROBOLOG_DEBUG_FMT` | debug | 调试信息 |
| `ROBOLOG_INFO_FMT` | info | 正常运行信息 |
| `ROBOLOG_WARN_FMT` | warn | 警告（接近限位、超时） |
| `ROBOLOG_ERROR_FMT` | error | 错误（求解失败、通信中断） |
| `ROBOLOG_CRITICAL_FMT` | critical | 严重错误（碰撞、紧急停机） |

## 生成的文件

运行后在 `logs/` 目录下生成（每日轮转 + 大小轮转模式）：

```
logs/
├── algorithm_2025-01-15.log            # 运行日志：当天主文件
├── algorithm_2025-01-15_1.log          # 当天第 1 次大小轮转
├── algorithm_error_2025-01-15.log      # 错误日志：当天主文件
└── algorithm_error_2025-01-15_1.log    # 错误日志：大小轮转
```

## 常见自定义

### 只要文件不要控制台

```cpp
config.console_enabled = false;
```

### 调整日志格式

修改 `log/src/robo_log.cpp` 中 `set_pattern()` 的参数。spdlog 格式说明：

| 占位符 | 含义 |
|--------|------|
| `%Y-%m-%d %H:%M:%S.%e` | 时间戳（含毫秒） |
| `%^%l%$` | 带颜色的级别名 |
| `%t` | 线程 ID |
| `%s:%#` | 文件名:行号 |
| `%v` | 日志消息 |

### 不需要错误日志文件

```cpp
config.error_file_enabled = false;
```

### 不需要每日轮转（只要大小轮转）

```cpp
config.daily_rotating_enabled = false;  // 使用 rotating_file_sink
config.file_enabled = true;
config.max_file_size = 50 * 1024 * 1024;
config.max_files = 10;
// 文件名: algorithm.log, algorithm.log.1, algorithm.log.2, ...
```

## 配置参数速查

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `level` | `info` | 全局日志过滤级别 |
| `console_enabled` | `true` | 是否输出到控制台（warn 及以上） |
| `file_enabled` | `true` | 是否启用运行日志文件（大小轮转模式） |
| `file_path` | `"logs/robot.log"` | 运行日志文件路径 |
| `max_file_size` | 50MB | 大小轮转模式：单文件上限 |
| `max_files` | 10 | 大小轮转模式：最大保留文件数 |
| `daily_rotating_enabled` | `false` | 是否启用每日轮转 + 大小轮转 |
| `daily_max_file_size` | 50MB | 每日轮转模式：单文件上限 |
| `daily_max_files` | 10 | 每日轮转模式：同一天最大保留文件数 |
| `error_file_enabled` | `true` | 是否启用错误日志单独文件 |
| `error_file_path` | `"logs/robot_error.log"` | 错误日志文件路径 |
| `error_max_file_size` | 50MB | 错误日志：单文件上限 |
| `error_max_files` | 10 | 错误日志：同一天最大保留文件数 |
| `async_enabled` | `true` | 是否启用异步模式 |
| `async_queue_size` | 65536 | 异步队列大小 |
| `async_thread_count` | 2 | 异步线程数 |
| `rotation_hour` | 0 | 每日轮转时刻（小时，0-23） |
| `rotation_minute` | 0 | 每日轮转时刻（分钟，0-59） |

## 注意事项

1. **日志目录自动创建**：`init()` 会根据 `file_path` 和 `error_file_path` 自动创建父目录，无需手动调用 `create_directories`

2. **不要 `#include "daily_rotating_file_sink.h"`**：它已被 `robo_log.h` 内部包含，用户代码只需 `#include "robo_log.h"`

3. **线程安全**：所有 RoboLog 方法都是 static + 线程安全的，可在任意线程直接调用

4. **异步模式下的日志丢失**：程序崩溃时，异步队列中未写入的日志会丢失。调试阶段建议用 `async_enabled = false`
