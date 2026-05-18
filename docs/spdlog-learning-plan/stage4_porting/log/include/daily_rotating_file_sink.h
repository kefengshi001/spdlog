/**
 * @file daily_rotating_file_sink.h
 * @brief 每日轮转文件 Sink — 结合日期轮转和大小轮转
 *
 * 自定义 spdlog sink，组合两种轮转策略:
 *   1. 按天轮转: 跨日时自动创建新文件(文件名含日期)
 *   2. 按大小轮转: 同一天内文件过大时自动切分(文件名含序号)
 *
 * 文件命名规则:
 *   输入: "logs/robot.log"
 *   当天当前文件:      logs/robot_2025-01-15.log
 *   当天第1次大小轮转: logs/robot_2025-01-15_1.log
 *   当天第2次大小轮转: logs/robot_2025-01-15_2.log
 *   新的一天:          logs/robot_2025-01-16.log
 *
 * 设计思路:
 *   spdlog 内置的 daily_file_sink 只按天轮转，rotating_file_sink 只按大小轮转。
 *   本 sink 继承 base_sink<Mutex>，组合两者的逻辑:
 *   - 参照 daily_file_sink 的时间检查: msg.time >= rotation_tp_
 *   - 参照 rotating_file_sink 的大小检查: current_size_ + new_size > max_size_
 *   - 参照 rotating_file_sink 的级联重命名: rotate_()
 *
 * 线程安全:
 *   模板参数 Mutex 控制线程安全策略:
 *   - daily_rotating_file_sink_mt: 使用 std::mutex，多线程安全
 *   - daily_rotating_file_sink_st: 使用 null_mutex，单线程无锁
 */

#pragma once

#include <spdlog/common.h>
#include <spdlog/details/file_helper.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/details/os.h>
#include <spdlog/sinks/base_sink.h>

#include <chrono>
#include <cstdio>
#include <mutex>
#include <string>

namespace spdlog {
namespace sinks {

/**
 * @class daily_rotating_file_sink
 * @brief 每日轮转 + 大小轮转的组合文件 sink
 *
 * @tparam Mutex 互斥锁类型(std::mutex 或 details::null_mutex)
 *
 * 轮转策略:
 *   每次 sink_it_() 写入前，按优先级检查:
 *   1. 日期变更检查: 如果 msg.time 已跨过当天轮转时刻(默认 00:00)
 *      → 关闭旧文件，打开新日期文件，重置大小计数器
 *   2. 大小溢出检查: 如果 current_size_ + 新消息大小 > max_size_
 *      → 执行大小轮转(级联重命名)，打开新文件，重置大小计数器
 *   3. 正常写入
 */
template <typename Mutex>
class daily_rotating_file_sink final : public base_sink<Mutex> {
public:
    /**
     * @brief 构造函数
     *
     * @param base_filename 基础文件名(如 "logs/robot.log")
     *                     实际文件名会插入日期: "logs/robot_2025-01-15.log"
     * @param max_size      单文件最大大小(字节)，超过后触发大小轮转
     * @param max_files     同一天内最大保留文件数(含当前文件)
     *                     例如 max_files=3: 当前文件 + _1.log + _2.log
     *                     最旧的文件会被删除
     * @param rotation_hour   每日轮转时刻-小时(0-23，默认 0)
     * @param rotation_minute 每日轮转时刻-分钟(0-59，默认 0)
     * @param truncate        是否截断已有文件(默认 false，追加模式)
     *
     * @throw spdlog_ex 如果 rotation_hour/minute 参数无效
     *
     * 初始化流程:
     *   1. 计算当前日期的文件名(如 robot_2025-01-15.log)
     *   2. 打开该文件(追加或截断)
     *   3. 计算下一次轮转时间点(明天 00:00)
     *   4. 记录当前文件大小(用于后续大小轮转判断)
     */
    daily_rotating_file_sink(filename_t base_filename,
                             std::size_t max_size,
                             std::size_t max_files,
                             int rotation_hour = 0,
                             int rotation_minute = 0,
                             bool truncate = false)
        : base_filename_(std::move(base_filename)),
          max_size_(max_size),
          max_files_(max_files > 0 ? max_files : 1),  // 至少保留 1 个文件
          rotation_h_(rotation_hour),
          rotation_m_(rotation_minute),
          current_size_(0),
          file_helper_() {
        // 参数校验
        if (rotation_hour < 0 || rotation_hour > 23 || rotation_minute < 0 ||
            rotation_minute > 59) {
            throw_spdlog_ex("daily_rotating_file_sink: Invalid rotation time");
        }
        if (max_size == 0) {
            throw_spdlog_ex("daily_rotating_file_sink: max_size must be > 0");
        }

        // 计算当前日期的文件名并打开
        auto now = log_clock::now();
        const auto filename = calc_filename(base_filename_, now_tm(now), 0);
        file_helper_.open(filename, truncate);

        // 如果是追加模式，初始化 current_size_ 为文件已有大小
        if (!truncate) {
            current_size_ = static_cast<std::size_t>(file_helper_.size());
        }

        // 计算下一次每日轮转时间点
        rotation_tp_ = next_rotation_tp_();
    }

    /**
     * @brief 获取当前正在写入的文件名
     * @return 当前文件的完整路径
     */
    filename_t filename() {
        std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
        return file_helper_.filename();
    }

protected:
    /**
     * @brief 写入日志消息(核心逻辑)
     *
     * 处理流程:
     *   1. 检查是否跨日 → 执行每日轮转
     *   2. 检查是否超大小 → 执行大小轮转
     *   3. 格式化并写入消息
     *   4. 更新大小计数器
     *
     * @param msg spdlog 日志消息(包含时间戳、级别、内容等)
     *
     * @note 此方法由 base_sink::log() 调用，调用时已持有 mutex_ 锁
     */
    void sink_it_(const details::log_msg &msg) override {
        auto time = msg.time;

        // ---- 检查 1: 每日轮转(日期变更) ----
        if (time >= rotation_tp_) {
            // 关闭旧日期的文件
            file_helper_.close();

            // 打开新日期的文件(index=0，即当天主文件)
            const auto new_filename = calc_filename(base_filename_, now_tm(time), 0);
            file_helper_.open(new_filename, false);
            current_size_ = 0;

            // 计算下一次轮转时间(明天同一时刻)
            rotation_tp_ = next_rotation_tp_();
        }

        // ---- 格式化日志消息 ----
        memory_buf_t formatted;
        base_sink<Mutex>::formatter_->format(msg, formatted);
        auto new_size = current_size_ + formatted.size();

        // ---- 检查 2: 大小轮转(单日文件过大) ----
        if (new_size > max_size_) {
            file_helper_.flush();
            // 二次确认文件确实非空(防止磁盘满时的异常)
            if (file_helper_.size() > 0) {
                rotate_();
                new_size = formatted.size();
            }
        }

        // ---- 写入日志 ----
        file_helper_.write(formatted);
        current_size_ = new_size;
    }

    /**
     * @brief 刷新缓冲区
     * @note 此方法由 base_sink::flush() 调用，调用时已持有 mutex_ 锁
     */
    void flush_() override { file_helper_.flush(); }

private:
    /**
     * @brief 计算带日期和序号的文件名
     *
     * 文件命名规则:
     *   输入: base_filename = "logs/robot.log", date = 2025-01-15
     *   index=0: "logs/robot_2025-01-15.log"       (当天主文件)
     *   index=1: "logs/robot_2025-01-15_1.log"     (第1次大小轮转)
     *   index=2: "logs/robot_2025-01-15_2.log"     (第2次大小轮转)
     *
     * @param base_filename 基础文件名
     * @param now_tm 当前时间的 tm 结构体
     * @param index 轮转序号(0=主文件，>0=轮转文件)
     * @return 完整的文件名
     */
    static filename_t calc_filename(const filename_t &base_filename, const tm &now_tm,
                                    std::size_t index) {
        filename_t basename, ext;
        std::tie(basename, ext) = details::file_helper::split_by_extension(base_filename);

        if (index == 0) {
            // 当天主文件: robot_2025-01-15.log
            return fmt_lib::format(
                SPDLOG_FMT_STRING(SPDLOG_FILENAME_T("{}_{:04d}-{:02d}-{:02d}{}")), basename,
                now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday, ext);
        } else {
            // 轮转文件: robot_2025-01-15_1.log
            return fmt_lib::format(
                SPDLOG_FMT_STRING(SPDLOG_FILENAME_T("{}_{:04d}-{:02d}-{:02d}_{}{}")), basename,
                now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday, index, ext);
        }
    }

    /**
     * @brief 将时间点转换为本地时间的 tm 结构体
     * @param tp 时间点
     * @return 本地时间的 tm 结构体
     */
    static tm now_tm(log_clock::time_point tp) {
        time_t tnow = log_clock::to_time_t(tp);
        return spdlog::details::os::localtime(tnow);
    }

    /**
     * @brief 计算下一次每日轮转的时间点
     *
     * 逻辑:
     *   1. 取当前时间，设置为今天 rotation_h_:rotation_m_:00
     *   2. 如果这个时间点还在未来 → 返回它(今天还没到轮转时刻)
     *   3. 如果这个时间点已经过去 → 加 24 小时(明天的轮转时刻)
     *
     * @return 下一次轮转的精确时间点
     */
    log_clock::time_point next_rotation_tp_() {
        auto now = log_clock::now();
        tm date = now_tm(now);
        date.tm_hour = rotation_h_;
        date.tm_min = rotation_m_;
        date.tm_sec = 0;
        auto rotation_time = log_clock::from_time_t(std::mktime(&date));
        if (rotation_time > now) {
            return rotation_time;
        }
        return {rotation_time + std::chrono::hours(24)};
    }

    /**
     * @brief 执行大小轮转
     *
     * 轮转策略(参照 rotating_file_sink 的级联重命名):
     *   假设 max_files=4，当前有: robot_2025-01-15.log, _1.log, _2.log
     *   轮转过程:
     *     1. 删除 robot_2025-01-15_3.log (如果存在，已达到上限)
     *     2. 重命名 _2.log → _3.log
     *     3. 重命名 _1.log → _2.log
     *     4. 重命名 主文件 → _1.log
     *     5. 创建新的 主文件(truncate)
     *
     * 轮转后: robot_2025-01-15.log(新空文件), _1.log(刚写满的), _2.log, _3.log
     *
     * @note 调用时已持有 mutex_ 锁
     */
    void rotate_() {
        // 获取当前日期信息(用于构造文件名)
        auto now = log_clock::now();
        tm date = now_tm(now);

        // 关闭当前文件
        file_helper_.close();

        // 从最旧的文件开始删除/重命名
        // max_files_ 包含当前文件，所以轮转文件序号从 max_files_-1 开始
        auto target_index = max_files_ - 1;

        // 删除最旧的文件(如果存在)
        auto oldest = calc_filename(base_filename_, date, target_index);
        details::os::remove_if_exists(oldest);

        // 级联重命名: {n-1} → {n}, {n-2} → {n-1}, ..., {0} → {1}
        for (auto i = target_index; i > 1; --i) {
            auto src = calc_filename(base_filename_, date, i - 1);
            auto dst = calc_filename(base_filename_, date, i);
            if (details::os::path_exists(src)) {
                details::os::rename(src, dst);
            }
        }

        // 将当前主文件(index=0)重命名为 _1.log
        auto current = calc_filename(base_filename_, date, 0);
        if (details::os::path_exists(current)) {
            auto rotated = calc_filename(base_filename_, date, 1);
            details::os::rename(current, rotated);
        }

        // 创建新的主文件(truncate=true)
        file_helper_.reopen(true);
        current_size_ = 0;
    }

    // ---- 成员变量 ----

    filename_t base_filename_;          ///< 基础文件名(不含日期和序号)
    std::size_t max_size_;              ///< 单文件最大大小(字节)
    std::size_t max_files_;             ///< 同一天内最大保留文件数
    int rotation_h_;                    ///< 每日轮转时刻-小时(0-23)
    int rotation_m_;                    ///< 每日轮转时刻-分钟(0-59)
    log_clock::time_point rotation_tp_; ///< 下一次每日轮转的时间点
    std::size_t current_size_;          ///< 当前文件已写入大小(内存中跟踪，避免频繁 stat)
    details::file_helper file_helper_;  ///< 文件 I/O 辅助类
};

// ---- 类型别名 ----

/// 多线程安全版本(使用 std::mutex)
using daily_rotating_file_sink_mt = daily_rotating_file_sink<std::mutex>;

/// 单线程版本(使用 null_mutex，无锁开销)
using daily_rotating_file_sink_st = daily_rotating_file_sink<details::null_mutex>;

}  // namespace sinks
}  // namespace spdlog
