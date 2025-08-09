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
#include <format>
#include <string>
#include <type_traits>
#include <concepts>

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

        // Original core logging functions (backward compatibility)
        void trace(std::string_view msg, const LogContext& ctx = {}, std::source_location loc = std::source_location::current()) noexcept;
        void debug(std::string_view msg, const LogContext& ctx = {}, std::source_location loc = std::source_location::current()) noexcept;
        void info(std::string_view msg, const LogContext& ctx = {}, std::source_location loc = std::source_location::current()) noexcept;
        void success(std::string_view msg, const LogContext& ctx = {}, std::source_location loc = std::source_location::current()) noexcept;
        void warning(std::string_view msg, const LogContext& ctx = {}, std::source_location loc = std::source_location::current()) noexcept;
        void error(std::string_view msg, const LogContext& ctx = {}, std::source_location loc = std::source_location::current()) noexcept;
        void critical(std::string_view msg, const LogContext& ctx = {}, std::source_location loc = std::source_location::current()) noexcept;

        // Internal namespace for implementation details
        namespace detail {
            // Helper to check if a type is LogContext
            template<typename T>
            constexpr bool is_log_context_v = std::is_same_v<std::remove_cvref_t<T>, LogContext>;

            // Base case: no arguments
            template<typename... Args>
            struct last_arg_helper {
                static constexpr bool is_context = false;
                using type = void;
            };

            // Recursive case: extract last argument type
            template<typename T>
            struct last_arg_helper<T> {
                static constexpr bool is_context = is_log_context_v<T>;
                using type = T;
            };

            template<typename T, typename... Rest>
            struct last_arg_helper<T, Rest...> : last_arg_helper<Rest...> {};

            // Helper to get the last argument if it's a context
            template<typename... Args>
            LogContext get_context_if_present(Args&&... args [[maybe_unused]] ) {
                if constexpr (sizeof...(Args) == 0) {
                    return {};
                }
                else if constexpr (last_arg_helper<Args...>::is_context) {
                    return std::get<sizeof...(Args) - 1>(std::forward_as_tuple(args...));
                }
                else {
                    return {};
                }
            }

            // Format message without the context parameter
            template<typename... Args>
            std::string format_message(std::string_view fmt, Args&&... args) {
                if constexpr (sizeof...(Args) == 0) {
                    return std::string(fmt);
                }
                else if constexpr (last_arg_helper<Args...>::is_context) {
                    // Last arg is context, exclude it from formatting
                    if constexpr (sizeof...(Args) == 1) {
                        // Only context, no format args
                        return std::string(fmt);
                    }
                    else {
                        // Format without the last argument
                        return[&fmt]<std::size_t... Is>(std::index_sequence<Is...>, auto&& tuple) {
                            return std::vformat(fmt, std::make_format_args(std::get<Is>(tuple)...));
                        }(std::make_index_sequence<sizeof...(Args) - 1>{}, std::forward_as_tuple(args...));
                    }
                }
                else {
                    // No context, use all args for formatting
                    return std::vformat(fmt, std::make_format_args(args...));
                }
            }
        }

        // Formatted logging functions - handle both simple strings and format strings
        template<typename... Args>
        void trace_fmt(std::string_view fmt, Args&&... args) noexcept {
            try {
                if constexpr (sizeof...(Args) == 0) {
                    trace(fmt);
                }
                else if constexpr (sizeof...(Args) == 1 && detail::last_arg_helper<Args...>::is_context) {
                    // Single argument that is a context
                    trace(fmt, detail::get_context_if_present(args...));
                }
                else {
                    auto formatted = detail::format_message(fmt, args...);
                    auto ctx = detail::get_context_if_present(args...);
                    trace(formatted, ctx);
                }
            }
            catch (...) {}
        }

        template<typename... Args>
        void debug_fmt(std::string_view fmt, Args&&... args) noexcept {
            try {
                if constexpr (sizeof...(Args) == 0) {
                    debug(fmt);
                }
                else if constexpr (sizeof...(Args) == 1 && detail::last_arg_helper<Args...>::is_context) {
                    debug(fmt, detail::get_context_if_present(args...));
                }
                else {
                    auto formatted = detail::format_message(fmt, args...);
                    auto ctx = detail::get_context_if_present(args...);
                    debug(formatted, ctx);
                }
            }
            catch (...) {}
        }

        template<typename... Args>
        void info_fmt(std::string_view fmt, Args&&... args) noexcept {
            try {
                if constexpr (sizeof...(Args) == 0) {
                    info(fmt);
                }
                else if constexpr (sizeof...(Args) == 1 && detail::last_arg_helper<Args...>::is_context) {
                    info(fmt, detail::get_context_if_present(args...));
                }
                else {
                    auto formatted = detail::format_message(fmt, args...);
                    auto ctx = detail::get_context_if_present(args...);
                    info(formatted, ctx);
                }
            }
            catch (...) {}
        }

        template<typename... Args>
        void success_fmt(std::string_view fmt, Args&&... args) noexcept {
            try {
                if constexpr (sizeof...(Args) == 0) {
                    success(fmt);
                }
                else if constexpr (sizeof...(Args) == 1 && detail::last_arg_helper<Args...>::is_context) {
                    success(fmt, detail::get_context_if_present(args...));
                }
                else {
                    auto formatted = detail::format_message(fmt, args...);
                    auto ctx = detail::get_context_if_present(args...);
                    success(formatted, ctx);
                }
            }
            catch (...) {}
        }

        template<typename... Args>
        void warning_fmt(std::string_view fmt, Args&&... args) noexcept {
            try {
                if constexpr (sizeof...(Args) == 0) {
                    warning(fmt);
                }
                else if constexpr (sizeof...(Args) == 1 && detail::last_arg_helper<Args...>::is_context) {
                    warning(fmt, detail::get_context_if_present(args...));
                }
                else {
                    auto formatted = detail::format_message(fmt, args...);
                    auto ctx = detail::get_context_if_present(args...);
                    warning(formatted, ctx);
                }
            }
            catch (...) {}
        }

        template<typename... Args>
        void error_fmt(std::string_view fmt, Args&&... args) noexcept {
            try {
                if constexpr (sizeof...(Args) == 0) {
                    error(fmt);
                }
                else if constexpr (sizeof...(Args) == 1 && detail::last_arg_helper<Args...>::is_context) {
                    error(fmt, detail::get_context_if_present(args...));
                }
                else {
                    auto formatted = detail::format_message(fmt, args...);
                    auto ctx = detail::get_context_if_present(args...);
                    error(formatted, ctx);
                }
            }
            catch (...) {}
        }

        template<typename... Args>
        void critical_fmt(std::string_view fmt, Args&&... args) noexcept {
            try {
                if constexpr (sizeof...(Args) == 0) {
                    critical(fmt);
                }
                else if constexpr (sizeof...(Args) == 1 && detail::last_arg_helper<Args...>::is_context) {
                    critical(fmt, detail::get_context_if_present(args...));
                }
                else {
                    auto formatted = detail::format_message(fmt, args...);
                    auto ctx = detail::get_context_if_present(args...);
                    critical(formatted, ctx);
                }
            }
            catch (...) {}
        }

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

// ============================================================================
// MACRO DEFINITIONS
// ============================================================================

// Debug build - all macros active with format support
#if defined(ASHBORN_DEBUG) || defined(_DEBUG)

// Simple versions (backward compatible)
#define print_t(...) ::AshCore::Logger::trace_fmt(__VA_ARGS__)
#define print_d(...) ::AshCore::Logger::debug_fmt(__VA_ARGS__)
#define print_i(...) ::AshCore::Logger::info_fmt(__VA_ARGS__)
#define print_s(...) ::AshCore::Logger::success_fmt(__VA_ARGS__)
#define print_w(...) ::AshCore::Logger::warning_fmt(__VA_ARGS__)
#define print_e(...) ::AshCore::Logger::error_fmt(__VA_ARGS__)
#define print_c(...) ::AshCore::Logger::critical_fmt(__VA_ARGS__)

// Release build - skip trace and debug
#elif defined(ASHBORN_RELEASE) || defined(NDEBUG)
#define print_t(...) ((void)0)
#define print_d(...) ((void)0)
#define print_i(...) ::AshCore::Logger::info_fmt(__VA_ARGS__)
#define print_s(...) ::AshCore::Logger::success_fmt(__VA_ARGS__)
#define print_w(...) ::AshCore::Logger::warning_fmt(__VA_ARGS__)
#define print_e(...) ::AshCore::Logger::error_fmt(__VA_ARGS__)
#define print_c(...) ::AshCore::Logger::critical_fmt(__VA_ARGS__)

// Distribution build - all macros compile to nothing
#elif defined(ASHBORN_DIST)
#define print_t(...) ((void)0)
#define print_d(...) ((void)0)
#define print_i(...) ((void)0)
#define print_s(...) ((void)0)
#define print_w(...) ((void)0)
#define print_e(...) ((void)0)
#define print_c(...) ((void)0)

// Default to debug if nothing is defined
#else
#define print_t(...) ::AshCore::Logger::trace_fmt(__VA_ARGS__)
#define print_d(...) ::AshCore::Logger::debug_fmt(__VA_ARGS__)
#define print_i(...) ::AshCore::Logger::info_fmt(__VA_ARGS__)
#define print_s(...) ::AshCore::Logger::success_fmt(__VA_ARGS__)
#define print_w(...) ::AshCore::Logger::warning_fmt(__VA_ARGS__)
#define print_e(...) ::AshCore::Logger::error_fmt(__VA_ARGS__)
#define print_c(...) ::AshCore::Logger::critical_fmt(__VA_ARGS__)
#endif

// Utility macros for common patterns
#define LOG_INIT() ::AshCore::Logger::init()
#define LOG_SHUTDOWN() ::AshCore::Logger::shutdown()
#define LOG_FLUSH() ::AshCore::Logger::flush()