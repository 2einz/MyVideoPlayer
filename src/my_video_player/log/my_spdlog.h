#pragma once

#include <memory>
#include <string>
#include <vector>
#include <atomic>

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace my_log {

// ================= 模块定义 =================
#define LOG_MODULE(X)                                                                                                  \
    X(kPlayer)                                                                                                         \
    X(kDemux)                                                                                                          \
    X(kDecode)                                                                                                         \
    X(kThread)                                                                                                         \
    X(kUI)                                                                                                             \
    X(kController)                                                                                                     \
    X(kMediaState)                                                                                                     \
    X(kFFmpeg)

enum class LogModule {
#define X(name) name,
    LOG_MODULE(X)
#undef X
};

inline const char* module2str(LogModule m) {
    switch (m) {
#define X(name)                                                                                                        \
    case LogModule::name:                                                                                              \
        return #name + 1;
        LOG_MODULE(X)
#undef X
    default:
        return "Unknown";
    }
}

// ================= logger 单例 =================
inline std::shared_ptr<spdlog::logger>& logger_ref() {
    static std::shared_ptr<spdlog::logger> logger;
    return logger;
}

// ================= fallback =================
inline std::shared_ptr<spdlog::logger> get_logger() {
    auto& logger = logger_ref();

    if (!logger) {
        auto existing = spdlog::get("default");
        if (existing) {
            logger = existing;
        } else {
            logger = spdlog::stdout_color_mt("default");
        }
    }
    return logger;
}

// ================= 初始化 =================
inline void init(const std::string& log_file = "logs/player.log") {
    try {
        // 1️⃣ 线程池（异步日志）
        spdlog::init_thread_pool(8192, 1);

        // 2️⃣ sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);

        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_file, 5 * 1024 * 1024, 3);
        file_sink->set_level(spdlog::level::debug);

        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};

        // 3️⃣ 异步 logger
        auto logger = std::make_shared<spdlog::async_logger>("main", sinks.begin(), sinks.end(), spdlog::thread_pool(),
                                                             spdlog::async_overflow_policy::block);

        logger_ref() = logger;

        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);

        // 4️⃣ 格式
        spdlog::set_pattern("[%H:%M:%S.%e][%^%L%$][%@] %v");

        // 5️⃣ 全局级别
        spdlog::set_level(spdlog::level::debug);
        spdlog::flush_on(spdlog::level::warn);

    } catch (const std::exception& ex) {
        fprintf(stderr, "Logger init failed: %s\n", ex.what());
    }
}

inline void shutdown() {
    spdlog::shutdown();
}

} // namespace my_log

using LM = my_log::LogModule;

inline const char* short_file(const char* file) {
    const char* p = strrchr(file, '\\');
    if (!p)
        p = strrchr(file, '/');
    return p ? p + 1 : file;
}

// ================= 宏封装 =================

#ifndef NDEBUG

#define LOG_TRACE(module, fmt, ...)                                                                                    \
    do {                                                                                                               \
        if (auto l = my_log::get_logger())                                                                             \
            l->log(spdlog::source_loc{short_file(__FILE__), __LINE__, SPDLOG_FUNCTION}, spdlog::level::trace,          \
                   "[{}] " fmt, my_log::module2str(module), ##__VA_ARGS__);                                            \
    } while (0)

#define LOG_DEBUG(module, fmt, ...)                                                                                    \
    do {                                                                                                               \
        if (auto l = my_log::get_logger())                                                                             \
            l->log(spdlog::source_loc{short_file(__FILE__), __LINE__, SPDLOG_FUNCTION}, spdlog::level::debug,          \
                   "[{}] " fmt, my_log::module2str(module), ##__VA_ARGS__);                                            \
    } while (0)

#else

#define LOG_TRACE(module, fmt, ...)
#define LOG_DEBUG(module, fmt, ...)

#endif

// ---- 始终保留 ----

#define LOG_INFO(module, fmt, ...)                                                                                     \
    do {                                                                                                               \
        if (auto l = my_log::get_logger())                                                                             \
            l->log(spdlog::source_loc{short_file(__FILE__), __LINE__, SPDLOG_FUNCTION}, spdlog::level::info,           \
                   "[{}] " fmt, my_log::module2str(module), ##__VA_ARGS__);                                            \
    } while (0)

#define LOG_WARN(module, fmt, ...)                                                                                     \
    do {                                                                                                               \
        if (auto l = my_log::get_logger())                                                                             \
            l->log(spdlog::source_loc{short_file(__FILE__), __LINE__, SPDLOG_FUNCTION}, spdlog::level::warn,           \
                   "[{}] " fmt, my_log::module2str(module), ##__VA_ARGS__);                                            \
    } while (0)

#define LOG_ERROR(module, fmt, ...)                                                                                    \
    do {                                                                                                               \
        if (auto l = my_log::get_logger())                                                                             \
            l->log(spdlog::source_loc{short_file(__FILE__), __LINE__, SPDLOG_FUNCTION}, spdlog::level::err,            \
                   "[{}] " fmt, my_log::module2str(module), ##__VA_ARGS__);                                            \
    } while (0)

#define LOG_CRITICAL(module, fmt, ...)                                                                                 \
    do {                                                                                                               \
        if (auto l = my_log::get_logger())                                                                             \
            l->log(spdlog::source_loc{short_file(__FILE__), __LINE__, SPDLOG_FUNCTION}, spdlog::level::critical,       \
                   "[{}] " fmt, my_log::module2str(module), ##__VA_ARGS__);                                            \
    } while (0)