# 阶段 3 设计文档: 高级用法

## 自定义 Sink 设计

### Sink 接口
所有自定义 Sink 需要继承 `spdlog::sinks::base_sink<Mutex>` 并实现：
- `sink_it_(const spdlog::details::log_msg &msg)` — 处理日志消息
- `flush_()` — 刷新缓冲区

### 常见自定义 Sink 场景
1. **回调 Sink** — 将日志发送到自定义回调函数
2. **环形缓冲 Sink** — 保留最近 N 条日志
3. **网络 Sink** — 发送到远程服务器
4. **数据库 Sink** — 写入数据库

## 自定义 Formatter 设计

### Formatter 接口
继承 `spdlog::formatter` 并实现：
- `format(const spdlog::details::log_msg &msg, spdlog::memory_buf_t &dest)` — 格式化消息
- `clone()` — 克隆 Formatter

### 常见自定义 Formatter 场景
1. **JSON 格式** — 输出 JSON 格式日志
2. **CSV 格式** — 输出 CSV 格式日志
3. **自定义时间格式** — 特殊时间戳格式

## 异步日志调优

### 关键参数
| 参数 | 说明 | 推荐值 |
|------|------|--------|
| 队列大小 | 缓冲区容量 | 8192+ |
| 线程数 | 后台写入线程 | 1-2 |
| 溢出策略 | 队列满时行为 | block_retry |

### 溢出策略
- `block_retry` — 阻塞等待（默认，最安全）
- `discard_log` — 丢弃新消息（最快）
- `overrun_oldest` — 覆盖最旧消息

## 日志轮转策略

### 按大小轮转
- 适用于：固定大小的日志文件
- 参数：最大文件大小、备份文件数

### 按时间轮转
- 适用于：按日期/小时分割的日志
- 参数：轮转时间点

### 按大小+时间组合
- 适用于：需要同时控制大小和时间

## 多线程安全

### 线程安全 Logger
- 使用 `_mt` 后缀（如 `stdout_color_sink_mt`）
- 内部使用互斥锁保护

### 单线程 Logger
- 使用 `_st` 后缀
- 无锁，性能更高

### 最佳实践
- 多线程环境使用 `_mt` 后缀
- 避免在日志回调中再次记录日志
- 使用 `flush()` 确保日志写入

## 性能优化

### 条件日志
- 使用 `SPDLOG_LOGGER_TRACE` 等宏
- 在日志级别不足时避免格式化开销

### 批量写入
- 异步日志天然支持批量写入
- 减少 I/O 操作次数

### 内存池
- spdlog 使用内存池减少分配开销
- 默认启用，无需额外配置
