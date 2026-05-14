# 阶段 4 设计文档: 移植到机械臂控制系统

## RoboLog 封装层设计

### 设计目标
1. 简化机械臂日志使用
2. 统一日志格式和分级
3. 隔离 spdlog 实现细节
4. 支持运行时配置

### 接口设计
```cpp
class RoboLog {
public:
    // 初始化
    static void init(const RoboLogConfig &config);

    // 日志接口
    static void trace(const std::string &msg);
    static void debug(const std::string &msg);
    static void info(const std::string &msg);
    static void warn(const std::string &msg);
    static void error(const std::string &msg);
    static void critical(const std::string &msg);

    // 格式化接口
    template<typename... Args>
    static void trace(fmt::format_string<Args...> fmt, Args &&...args);

    // 控制接口
    static void set_level(spdlog::level::level_enum level);
    static void flush();
    static void shutdown();
};
```

### 日志分级策略
| 日志类型 | 级别 | 频率 | 输出目标 |
|---------|------|------|---------|
| 实时数据 | trace | 1kHz+ | 异步文件 |
| 算法调试 | debug | 中频 | 异步文件 |
| 运行事件 | info | 低频 | 同步文件+控制台 |
| 警告信息 | warn | 低频 | 同步文件+控制台 |
| 故障报警 | error | 极低频 | 同步文件+错误文件+控制台 |
| 紧急事件 | critical | 极低频 | 同步文件+错误文件+控制台 |

### 输出策略
```
控制台 (warn+)
  │
  ├──► 运行日志文件 (所有级别, 按大小轮转 10MB x 5)
  │
  └──► 错误日志文件 (error+)
```

## 性能要求

### 实时控制循环
- 控制周期: 1ms (1kHz)
- 日志写入不能阻塞控制线程
- 高频数据使用异步日志

### 性能指标
| 指标 | 目标值 |
|------|--------|
| 日志写入延迟 | < 10μs (异步) |
| 内存占用 | < 10MB |
| CPU 占用 | < 1% |
| 文件 I/O | 不阻塞控制线程 |

## 配置管理

### 配置文件格式
```json
{
    "level": "info",
    "console": true,
    "file": {
        "enabled": true,
        "path": "logs/robot.log",
        "max_size_mb": 10,
        "max_files": 5
    },
    "error_file": {
        "enabled": true,
        "path": "logs/robot_error.log"
    },
    "async": {
        "enabled": true,
        "queue_size": 8192,
        "thread_count": 1
    }
}
```

## 集成方式

### header-only 模式
- 只需复制 include/spdlog/ 目录
- 无链接依赖
- 编译时间略长

### CMake 集成
```cmake
# 方式1: add_subdirectory
add_subdirectory(third_party/spdlog)

# 方式2: find_package
find_package(spdlog REQUIRED)

# 方式3: FetchContent
include(FetchContent)
FetchContent_Declare(spdlog ...)
```

## 部署考虑

### 目标平台
- 嵌入式 Linux
- RTOS
- 工控机

### 文件系统
- 确保日志目录有写权限
- 考虑存储空间限制
- 定期清理旧日志

## 团队使用规范

### 日志级别使用规范
- `trace`: 仅用于高频实时数据
- `debug`: 开发调试阶段使用
- `info`: 正常运行事件
- `warn`: 需要关注但不影响运行
- `error`: 功能失败，需要处理
- `critical`: 紧急情况，立即处理

### 格式规范
- 包含时间戳
- 包含日志级别
- 包含模块名称
- 包含有意义的上下文信息
