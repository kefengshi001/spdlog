/**
 * spdlog 阶段 3: 日志轮转策略 (Log Rotation Strategies)
 *
 * 本文件演示 spdlog 提供的三种日志轮转机制:
 *   1. 按大小轮转 (Rotating File Sink)  - 文件达到指定大小时创建新文件
 *   2. 按时间轮转 (Daily/Hourly Sink)   - 按固定时间间隔创建新文件
 *   3. 组合策略 (自定义 Sink)           - 先按时间分割，再限制单文件大小
 *
 * 日志轮转的目的:
 *   - 防止单个日志文件无限增长占满磁盘
 *   - 便于日志归档和清理
 *   - 按时间分割便于定位特定时间段的问题
 *
 * 轮转后的文件命名:
 *   - 按大小: filename.log, filename.log.1, filename.log.2, ...
 *   - 按时间: filename_2025-01-15.log, filename_2025-01-16.log, ...
 */

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"   // 按大小轮转
#include "spdlog/sinks/daily_file_sink.h"       // 按天轮转
#include "spdlog/sinks/hourly_file_sink.h"      // 按小时轮转
#include "spdlog/sinks/base_sink.h"             // 自定义 sink 基类
#include "spdlog/details/file_helper.h"         // 文件操作辅助
#include "spdlog/details/os.h"                  // 系统时间相关

#include <iostream>
#include <cstdio>  // std::rename

/**
 * 演示 1: 按大小轮转 (Rotating File Sink)
 *
 * 当日志文件达到指定大小时，自动创建新文件:
 *   - rotating_size.log     ← 当前活跃文件
 *   - rotating_size.log.1   ← 最近一次轮转的文件
 *   - rotating_size.log.2   ← 更早的轮转文件
 *   - rotating_size.log.3   ← 最早的轮转文件(超过 max_files 会被删除)
 *
 * 参数说明:
 *   - filename:       日志文件路径
 *   - max_file_size:  单个文件最大大小(字节)
 *   - max_files:      保留的备份文件数量(不含当前活跃文件)
 *
 * 适用场景:
 *   - 磁盘空间有限的嵌入式设备
 *   - 需要严格控制日志占用空间的场景
 *   - 日志量大但不需要长期保留
 */
void rotating_by_size_demo() {
    std::cout << "\n=== 1. 按大小轮转 ===" << std::endl;

    // 创建按大小轮转的 logger
    // 参数: 文件名, 最大大小(1MB), 最多 3 个备份文件
    auto logger = spdlog::rotating_logger_mt(
        "rotating_size", "logs/rotating_size.log",
        1024 * 1024,  // 1MB = 1048576 字节
        3);            // 保留 3 个备份(.1, .2, .3)

    logger->info("Rotating by size: max 1MB, 3 backups");
    logger->info("Files: rotating_size.log, rotating_size.log.1, .2, .3");

    spdlog::drop("rotating_size");
}

/**
 * 演示 2: 按时间轮转
 *
 * spdlog 提供两种时间轮转 Sink:
 *
 *   Daily File Sink (每日轮转):
 *     - 每天在指定时间创建新文件
 *     - 文件名自动附加日期后缀: daily_2025-01-15.log
 *     - 参数: filename, rotation_hour, rotation_minute
 *     - 适合: 按天归档日志的常规应用
 *
 *   Hourly File Sink (每小时轮转):
 *     - 每小时创建新文件
 *     - 文件名自动附加时间后缀
 *     - 适合: 高频日志、需要精细时间定位的场景
 */
void rotating_by_time_demo() {
    std::cout << "\n=== 2. 按时间轮转 ===" << std::endl;

    // 每日轮转: 每天凌晨 2:30 创建新文件
    // 常见选择: 凌晨轮转，避免业务高峰期切换文件
    auto daily_logger = spdlog::daily_logger_mt(
        "daily", "logs/daily.log", 2, 30);  // 凌晨 2:30 轮转

    daily_logger->info("Daily rotation: new file at 02:30 every day");

    // 每小时轮转: 每小时自动创建新文件
    auto hourly_logger = spdlog::hourly_logger_mt(
        "hourly", "logs/hourly.log");

    hourly_logger->info("Hourly rotation: new file every hour");

    spdlog::drop("daily");
    spdlog::drop("hourly");
}

/**
 * 演示 3: 组合策略 - 先按时间分割，再限制单文件大小
 *
 * spdlog 没有内置这种组合 sink，需要自定义实现。
 *
 * 思路: 继承 base_sink，自行管理文件句柄:
 *   1. 每天轮转: 检查当前日期，跨天时打开新文件 (文件名带日期后缀)
 *   2. 大小轮转: 同一天内文件超过 max_size 时，轮转为 .1, .2, ...
 *
 * 文件命名示例 (max_size=1MB, max_files=3):
 *   logs/combo_2025-01-15.log       ← 当天活跃文件
 *   logs/combo_2025-01-15.log.1     ← 当天已轮转文件
 *   logs/combo_2025-01-15.log.2     ← 当天已轮转文件
 *   logs/combo_2025-01-14.log       ← 前一天的日志
 *
 * 实现要点:
 *   - 使用 spdlog::details::file_helper 管理文件 I/O
 *   - 使用 spdlog::details::os::localtime 获取当前时间
 *   - 每次写入前检查: 是否跨天 → 是否超出大小
 *   - 重命名链: file → file.1 → file.2 → ... (最旧的被删除)
 */
template <typename Mutex>
class daily_size_rotating_sink final : public spdlog::sinks::base_sink<Mutex> {
public:
    daily_size_rotating_sink(spdlog::filename_t base_filename,
                             std::size_t max_size,
                             std::size_t max_files,
                             int rotation_hour = 0,
                             int rotation_minute = 0)
        : base_filename_(std::move(base_filename)),
          max_size_(max_size),
          max_files_(max_files),
          rotation_hour_(rotation_hour),
          rotation_minute_(rotation_minute),
          current_size_(0) {
        auto now = spdlog::log_clock::now();
        current_day_ = day_of(now);
        open_new_day_file_(now);
    }

    ~daily_size_rotating_sink() { file_helper_.flush(); }

protected:
    void sink_it_(const spdlog::details::log_msg &msg) override {
        auto now = msg.time;

        // 1. 检查是否跨天 (先按时间分割)
        auto today = day_of(now);
        if (today != current_day_) {
            rotate_by_day_(now);
        }

        // 2. 格式化日志消息
        spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);
        auto msg_size = formatted.size();

        // 3. 检查当前文件是否超出大小限制 (再限制单文件大小)
        if (current_size_ + msg_size > max_size_) {
            rotate_by_size_();
        }

        // 4. 写入文件
        file_helper_.write(formatted);
        current_size_ += msg_size;
    }

    void flush_() override { file_helper_.flush(); }

private:
    using spdlog::sinks::base_sink<Mutex>::mutex_;
    using day_t = int;  // YYYYMMDD 格式的日期整数

    // 将 time_point 转换为 YYYYMMDD 整数，用于跨天检测
    day_t day_of(spdlog::log_clock::time_point tp) {
        auto t = spdlog::log_clock::to_time_t(tp);
        auto tm = spdlog::details::os::localtime(t);
        return (tm.tm_year + 1900) * 10000 + (tm.tm_mon + 1) * 100 + tm.tm_mday;
    }

    // 生成当天的文件名: basename_YYYY-MM-DD.ext
    spdlog::filename_t day_filename_(spdlog::log_clock::time_point tp) {
        auto t = spdlog::log_clock::to_time_t(tp);
        auto tm = spdlog::details::os::localtime(t);
        spdlog::filename_t basename, ext;
        std::tie(basename, ext) = spdlog::details::file_helper::split_by_extension(base_filename_);
        return fmt::format(SPDLOG_FMT_STRING(SPDLOG_FILENAME_T("{}_{:04d}-{:02d}-{:02d}{}")),
                           basename, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, ext);
    }

    // 打开当天的新文件
    void open_new_day_file_(spdlog::log_clock::time_point tp) {
        auto filename = day_filename_(tp);
        file_helper_.open(filename, false);  // 不截断，追加模式
        current_size_ = file_helper_.size();
        current_day_file_ = filename;
    }

    // 按时间轮转: 新的一天，打开新文件
    void rotate_by_day_(spdlog::log_clock::time_point now) {
        file_helper_.flush();
        current_day_ = day_of(now);
        open_new_day_file_(now);
    }

    // 按大小轮转: 当天文件超出 max_size，重命名为 .1, .2, ...
    void rotate_by_size_() {
        file_helper_.flush();

        // 删除最旧的备份 (如果已达到 max_files)
        if (max_files_ > 0) {
            auto oldest = fmt::format(SPDLOG_FILENAME_T("{}.{}"),
                                      current_day_file_, max_files_);
            std::remove(oldest.c_str());  // 忽略不存在的情况
        }

        // 逐个重命名: .N-1 → .N, .N-2 → .N-1, ..., file → .1
        for (auto i = max_files_; i > 1; --i) {
            auto src = fmt::format(SPDLOG_FILENAME_T("{}.{}"),
                                   current_day_file_, i - 1);
            auto dst = fmt::format(SPDLOG_FILENAME_T("{}.{}"),
                                   current_day_file_, i);
            std::rename(src.c_str(), dst.c_str());
        }

        // 当前活跃文件 → .1
        auto rotated = fmt::format(SPDLOG_FILENAME_T("{}.1"), current_day_file_);
        std::rename(current_day_file_.c_str(), rotated.c_str());

        // 打开新的活跃文件
        file_helper_.open(current_day_file_, false);
        current_size_ = 0;
    }

    spdlog::filename_t base_filename_;
    spdlog::filename_t current_day_file_;  // 当天的活跃文件路径
    std::size_t max_size_;                 // 单文件最大字节数
    std::size_t max_files_;                // 每天保留的备份文件数
    int rotation_hour_;
    int rotation_minute_;
    std::size_t current_size_;             // 当前文件已写入字节数
    day_t current_day_;                    // 当前日期 (YYYYMMDD)
    spdlog::details::file_helper file_helper_;
};

using daily_size_rotating_sink_mt = daily_size_rotating_sink<std::mutex>;

/**
 * 创建 "先按时间分割，再限制单文件大小" 的 logger
 *
 * 参数:
 *   name:            logger 名称
 *   filename:        基础文件名 (如 "logs/combo.log")
 *   max_size:        单文件最大字节数 (如 1MB = 1024*1024)
 *   max_files:       每天保留的最大备份文件数
 *   rotation_hour:   每日轮转的小时 (0-23)
 *   rotation_minute: 每日轮转的分钟 (0-59)
 */
std::shared_ptr<spdlog::logger> create_daily_size_logger(
    const std::string &name,
    const spdlog::filename_t &filename,
    std::size_t max_size,
    std::size_t max_files,
    int rotation_hour = 0,
    int rotation_minute = 0) {
    auto sink = std::make_shared<daily_size_rotating_sink_mt>(
        filename, max_size, max_files, rotation_hour, rotation_minute);
    auto logger = std::make_shared<spdlog::logger>(name, sink);
    spdlog::initialize_logger(logger);
    return logger;
}

/**
 * 演示 3: 组合策略 - 先按时间分割，再限制单文件大小
 *
 * 轮转逻辑:
 *   1. 每天自动切换到新文件 (文件名带日期后缀)
 *   2. 同一天内，如果文件超过 max_size，轮转为 .1, .2, ...
 *
 * 生成的文件结构:
 *   logs/daily_size_2025-01-15.log       ← 当天活跃文件
 *   logs/daily_size_2025-01-15.log.1     ← 当天第一个备份
 *   logs/daily_size_2025-01-15.log.2     ← 当天第二个备份
 *   logs/daily_size_2025-01-14.log       ← 前一天的日志
 */
void rotation_strategy_demo() {
    std::cout << "\n=== 3. 组合策略: 按时间 + 按大小轮转 ===" << std::endl;

    // 创建组合策略 logger
    // 参数: 文件名, 单文件最大 1KB, 每天保留 2 个备份, 每天 00:00 轮转
    auto logger = create_daily_size_logger(
        "daily_size", "logs/daily_size.log",
        1024*1024,  // 1MB (实际使用建议设为 1MB+)
        3,     // 每天最多保留 2 个备份
        0, 0); // 每天 00:00 轮转

    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    // 写入测试日志，触发大小轮转
    for (int i = 0; i < 50; ++i) {
        logger->info("组合策略测试日志 #{:03d} - 先按时间分割，再限制单文件大小", i);
    }

    logger->flush();
    spdlog::drop("daily_size");

    std::cout << "  已生成日志文件，请查看 logs/daily_size_*.log" << std::endl;
    std::cout << "  策略: 每天新文件 + 当天文件超 1KB 自动轮转为 .1, .2" << std::endl;
}

int main() {
    try {
        std::cout << "=== 日志轮转策略演示 ===" << std::endl;

        rotating_by_size_demo();
        rotating_by_time_demo();
        rotation_strategy_demo();

        std::cout << "\n=== 日志轮转策略演示完成 ===" << std::endl;
        spdlog::shutdown();  // 刷新所有 logger 缓冲区

    } catch (const spdlog::spdlog_ex &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
