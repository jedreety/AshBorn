#pragma once

#include <expected>
#include <string_view>
#include <filesystem>
#include <chrono>
#include <optional>
#include <functional>
#include <any>
#include <unordered_map>
#include <source_location>

namespace AshCore {

    // Error types
    enum class LogError {
        None = 0,
        AlreadyInitialized,
        NotInitialized,
        HandlerCreationFailed,
        HandlerNotFound,
		HandlerRemovalFailed,
        FileCreationFailed,
		FileFlushFailed,
        InvalidConfiguration,
        QueueFull,
        Unknown
    };

    // Log levels
    enum class LogLevel {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Success = 3,
        Warning = 4,
        Error = 5,
        Critical = 6
    };

    // Context data type
    using LogContext = std::unordered_map<std::string, std::any>;

    // Performance stats
    struct LogStats {
        std::size_t messages_logged;
        std::size_t messages_dropped;
        std::size_t handlers_active;
        double messages_per_second;
        bool queue_saturated;
    };

    // Handler configuration
    struct HandlerConfig {
        std::string name;
        LogLevel min_level = LogLevel::Trace;
        bool use_colors = true;
        bool show_timestamp = true;
        bool show_thread_id = false;
        bool structured_json = false;
    };

    // File handler configuration
    struct FileHandlerConfig : HandlerConfig {
        std::filesystem::path file_path;
        std::size_t max_file_size = 100 * 1024 * 1024; // 100MB
        bool auto_rotate = true;
    };

    // Core logging functions
    namespace Logger {
        // Initialization and shutdown
        [[nodiscard]] std::expected<void, LogError> init() noexcept;
        [[nodiscard]] std::expected<void, LogError> shutdown() noexcept;
        [[nodiscard]] bool is_initialized() noexcept;

        // Handler management
        [[nodiscard]] std::expected<void, LogError> add_console_handler(const HandlerConfig& config = {}) noexcept;
        [[nodiscard]] std::expected<void, LogError> add_file_handler(const FileHandlerConfig& config) noexcept;
        [[nodiscard]] std::expected<void, LogError> remove_handler(std::string_view name) noexcept;
        [[nodiscard]] std::expected<void, LogError> clear_handlers() noexcept;

        // Runtime configuration
        [[nodiscard]] std::expected<void, LogError> set_min_level(LogLevel level) noexcept;
        [[nodiscard]] std::expected<void, LogError> set_min_level_for_handler(std::string_view handler, LogLevel level) noexcept;
        [[nodiscard]] LogLevel get_min_level() noexcept;

        // Performance and monitoring
        [[nodiscard]] LogStats get_stats() noexcept;
        [[nodiscard]] std::expected<void, LogError> flush() noexcept;
        [[nodiscard]] std::expected<void, LogError> flush_handler(std::string_view handler) noexcept;

        // Core logging functions (with context support)
        void trace(std::string_view msg, const LogContext& ctx = {}, std::source_location loc = std::source_location::current()) noexcept;
        void debug(std::string_view msg, const LogContext& ctx = {}, std::source_location loc = std::source_location::current()) noexcept;
        void info(std::string_view msg, const LogContext& ctx = {}, std::source_location loc = std::source_location::current()) noexcept;
        void success(std::string_view msg, const LogContext& ctx = {}, std::source_location loc = std::source_location::current()) noexcept;
        void warning(std::string_view msg, const LogContext& ctx = {}, std::source_location loc = std::source_location::current()) noexcept;
        void error(std::string_view msg, const LogContext& ctx = {}, std::source_location loc = std::source_location::current()) noexcept;
        void critical(std::string_view msg, const LogContext& ctx = {}, std::source_location loc = std::source_location::current()) noexcept;

        // Utility functions
        [[nodiscard]] std::expected<void, LogError> rotate_file(std::string_view handler) noexcept;
        [[nodiscard]] std::expected<std::size_t, LogError> get_file_size(std::string_view handler) noexcept;

        // Advanced configuration
        [[nodiscard]] std::expected<void, LogError> set_queue_size(std::size_t size) noexcept;
        [[nodiscard]] std::expected<void, LogError> set_overflow_policy(bool drop_on_full) noexcept;
        [[nodiscard]] std::expected<void, LogError> enable_async(bool enable [[maybe_unused]] ) noexcept;

        // Benchmarking
        struct BenchmarkResult {
            double messages_per_second;
            std::chrono::nanoseconds avg_latency;
            std::chrono::nanoseconds min_latency;
            std::chrono::nanoseconds max_latency;
        };
        [[nodiscard]] std::expected<BenchmarkResult, LogError> benchmark(std::size_t num_messages = 100000) noexcept;
    }

} // namespace AshCore

// Macro definitions based on build configuration

// Helper macro for context support
#define ASHBORN_LOG_CTX(...) __VA_OPT__(, __VA_ARGS__)

// Debug build - all macros active
#if defined(ASHBORN_DEBUG) || defined(_DEBUG)
#define print_t(msg, ...) ::AshCore::Logger::trace(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#define print_d(msg, ...) ::AshCore::Logger::debug(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#define print_i(msg, ...) ::AshCore::Logger::info(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#define print_s(msg, ...) ::AshCore::Logger::success(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#define print_w(msg, ...) ::AshCore::Logger::warning(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#define print_e(msg, ...) ::AshCore::Logger::error(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#define print_c(msg, ...) ::AshCore::Logger::critical(msg ASHBORN_LOG_CTX(__VA_ARGS__))

// Release build - skip trace and debug
#elif defined(ASHBORN_RELEASE) || defined(NDEBUG)
#define print_t(msg, ...) ((void)0)
#define print_d(msg, ...) ((void)0)
#define print_i(msg, ...) ::AshCore::Logger::info(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#define print_s(msg, ...) ::AshCore::Logger::success(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#define print_w(msg, ...) ::AshCore::Logger::warning(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#define print_e(msg, ...) ::AshCore::Logger::error(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#define print_c(msg, ...) ::AshCore::Logger::critical(msg ASHBORN_LOG_CTX(__VA_ARGS__))

// Distribution build - all macros compile to nothing
#elif defined(ASHBORN_DIST)
#define print_t(msg, ...) ((void)0)
#define print_d(msg, ...) ((void)0)
#define print_i(msg, ...) ((void)0)
#define print_s(msg, ...) ((void)0)
#define print_w(msg, ...) ((void)0)
#define print_e(msg, ...) ((void)0)
#define print_c(msg, ...) ((void)0)

// Default to debug if nothing is defined
#else
#define print_t(msg, ...) ::AshCore::Logger::trace(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#define print_d(msg, ...) ::AshCore::Logger::debug(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#define print_i(msg, ...) ::AshCore::Logger::info(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#define print_s(msg, ...) ::AshCore::Logger::success(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#define print_w(msg, ...) ::AshCore::Logger::warning(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#define print_e(msg, ...) ::AshCore::Logger::error(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#define print_c(msg, ...) ::AshCore::Logger::critical(msg ASHBORN_LOG_CTX(__VA_ARGS__))
#endif

// Utility macros for common patterns
#define LOG_INIT() ::AshCore::Logger::init()
#define LOG_SHUTDOWN() ::AshCore::Logger::shutdown()
#define LOG_FLUSH() ::AshCore::Logger::flush()