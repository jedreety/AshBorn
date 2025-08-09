#include "ashbornpch.h"
#include <Logger.h>
#include <iostream>

namespace AshCore {

    namespace {
        // Internal state
        std::atomic<bool> g_initialized{ false };
        std::mutex g_init_mutex;
        LogLevel g_min_level = LogLevel::Trace;

        // Handler tracking
        struct HandlerInfo {
            std::string name;
            LogLevel min_level;
            bool is_file;
            std::filesystem::path file_path;
        };
        std::vector<HandlerInfo> g_handlers;

        // Convert between our types and Gem types
        Gem::LogLevel to_gem_level(LogLevel level) {
            return static_cast<Gem::LogLevel>(static_cast<int>(level));
        }

        LogLevel from_gem_level(Gem::LogLevel level) {
            return static_cast<LogLevel>(static_cast<int>(level));
        }

        // Convert LogContext to Gem::ContextMap
        Gem::ContextMap to_gem_context(const LogContext& ctx) {
            Gem::ContextMap gem_ctx;

            for (const auto& [key, value] : ctx) 
                gem_ctx[key] = value;
            
            return gem_ctx;
        }

        // Format strings for different log levels with colors
        std::string get_format_for_level(LogLevel level, bool use_colors, bool show_timestamp, bool show_thread) {
            std::string format;

            // Timestamp
            if (show_timestamp) {
                format += "%(time) ";
            }

            // Thread ID
            if (show_thread) {
                format += "[%(thread)] ";
            }

            // Level with colors
            if (use_colors) {
                switch (level) {
                case LogLevel::Trace:
                    format += "<dim>[TRACE] (%(file):%(line))</dim> %(message)";
                    break;
                case LogLevel::Debug:
                    format += "<cyan>[DEBUG]</cyan> %(message)";
                    break;
                case LogLevel::Info:
                    format += "<green>[INFO]</green> %(message)";
                    break;
                case LogLevel::Success:
                    format += "<bold><green>[SUCCESS]</green></bold> %(message)";
                    break;
                case LogLevel::Warning:
                    format += "<yellow>[WARN]</yellow> %(message)";
                    break;
                case LogLevel::Error:
                    format += "<red>[ERROR]</red> %(message) <dim>(%(file):%(line))</dim>";
                    break;
                case LogLevel::Critical:
                    format += "<bold><red>[CRITICAL]</red></bold> %(message) <dim>(%(file):%(line))</dim>";
                    break;
                }
            }
            else {
                switch (level) {
                case LogLevel::Trace:
                    format += "[TRACE] (%(file):%(line)) %(message)";
                    break;
                case LogLevel::Error:
                case LogLevel::Critical:
                    format += "[%(levelname)] %(message) (%(file):%(line))";
                    break;
                default:
                    format += "[%(levelname)] %(message)";
                    break;
                }
            }

            return format;
        }
    }

    namespace Logger {

        std::expected<void, LogError> init() noexcept {
            try {

                std::lock_guard lock(g_init_mutex);

                if (g_initialized.load()) 
                    return std::unexpected(LogError::AlreadyInitialized);

                // Create default console handler with colors
                HandlerConfig default_config{
                    .name = "console",
                    .min_level = LogLevel::Trace,
                    .use_colors = true,
                    .show_timestamp = false,
                    .show_thread_id = false,
                    .structured_json = false
                };

                // Build configuration using Gem::Logger - use a single chain
                auto config = Gem::ConfigTemplate::builder()
                    .name(default_config.name)
                    .level(to_gem_level(default_config.min_level))
                    .format("simple", get_format_for_level(LogLevel::Info,
                        default_config.use_colors,
                        default_config.show_timestamp,
                        default_config.show_thread_id))
                    .output("simple", Gem::StreamTarget::cout())
                    .build();

                auto result = Gem::Logger::instance().add_handler(std::move(config));
                if (result.is_err()) 
                    return std::unexpected(LogError::HandlerCreationFailed);

                g_handlers.push_back({ default_config.name, default_config.min_level, false, {} });

                g_initialized.store(true);
                return {};

            }
            catch (...) {
                return std::unexpected(LogError::Unknown);
            }
        }

        std::expected<void, LogError> shutdown() noexcept {
            try {

                std::lock_guard lock(g_init_mutex);

                if (!g_initialized.load())
                    return std::unexpected(LogError::NotInitialized);

				bool has_errors = false;
                for (const auto& handler : g_handlers) {
                    
                    auto remove_result = Gem::Logger::instance().remove_handler(handler.name);
                    if (remove_result.is_err()) has_errors = true;
                }
                g_handlers.clear();

                auto flush_result = Gem::get_file_cache().flush_all();

                Gem::Logger::instance().shutdown();

                g_initialized.store(false);

                if (flush_result.is_err()) 
					return std::unexpected(LogError::FileFlushFailed);
                
				if (has_errors) 
					return std::unexpected(LogError::HandlerRemovalFailed);

                return {};

            }
            catch (...) {
                return std::unexpected(LogError::Unknown);
            }
        }

        bool is_initialized() noexcept {
            return g_initialized.load();
        }

        std::expected<void, LogError> add_console_handler(const HandlerConfig& config) noexcept {
            try {

                if (!g_initialized.load()) 
                    return std::unexpected(LogError::NotInitialized);

                std::string handler_name = config.name.empty() ?
                    "console_" + std::to_string(g_handlers.size()) : config.name;

                // Create a simple format that will work for all levels
                std::string format_pattern = get_format_for_level(LogLevel::Info,
                    config.use_colors,
                    config.show_timestamp,
                    config.show_thread_id);

                // Build configuration with proper chaining
                auto builder = Gem::ConfigTemplate::builder()
                    .name(handler_name)
                    .level(to_gem_level(config.min_level))
                    .format("main", format_pattern);

                if (config.structured_json) 
                    builder = builder.structured(true);

                auto gem_config = builder
                    .output("main", Gem::StreamTarget::cout())
                    .build();

                auto result = Gem::Logger::instance().add_handler(std::move(gem_config));
                if (result.is_err()) 
                    return std::unexpected(LogError::HandlerCreationFailed);

                g_handlers.push_back({ handler_name, config.min_level, false, {} });
                return {};

            }
            catch (...) {
                return std::unexpected(LogError::Unknown);
            }
        }

        std::expected<void, LogError> add_file_handler(const FileHandlerConfig& config) noexcept {
            try {

                if (!g_initialized.load()) 
                    return std::unexpected(LogError::NotInitialized);

                std::string handler_name = config.name.empty() ?
                    "file_" + std::to_string(g_handlers.size()) : config.name;

                // Create format without colors for file
                std::string format_pattern = get_format_for_level(LogLevel::Info,
                    false,  // No colors in file
                    true,   // Always show timestamp in file
                    config.show_thread_id);

                // Build configuration with proper chaining
                auto builder = Gem::ConfigTemplate::builder()
                    .name(handler_name)
                    .level(to_gem_level(config.min_level))
                    .format("main", format_pattern);

                if (config.structured_json) 
                    builder = builder.structured(true);

                auto gem_config = builder
                    .output("main", config.file_path)
                    .build();

                auto result = Gem::Logger::instance().add_handler(std::move(gem_config));
                if (result.is_err()) 
                    return std::unexpected(LogError::HandlerCreationFailed);

                g_handlers.push_back({ handler_name, config.min_level, true, config.file_path });
                return {};

            }
            catch (...) {
                return std::unexpected(LogError::Unknown);
            }
        }

        std::expected<void, LogError> remove_handler(std::string_view name) noexcept {
            try {

                if (!g_initialized.load()) 
                    return std::unexpected(LogError::NotInitialized);

                auto it = std::find_if(g_handlers.begin(), g_handlers.end(),
                    [name](const HandlerInfo& h) { return h.name == name; });

                if (it == g_handlers.end()) 
                    return std::unexpected(LogError::HandlerNotFound);

                auto result = Gem::Logger::instance().remove_handler(name);
                if (result.is_err()) 
                    return std::unexpected(LogError::HandlerRemovalFailed);

                g_handlers.erase(it);
                return {};

            }
            catch (...) {
                return std::unexpected(LogError::Unknown);
            }
        }

        std::expected<void, LogError> clear_handlers() noexcept {
            try {

                if (!g_initialized.load()) 
                    return std::unexpected(LogError::NotInitialized);

				bool has_errors = false;
                for (const auto& handler : g_handlers) {

                    auto remove_result = Gem::Logger::instance().remove_handler(handler.name);
                    if (remove_result.is_err()) 
						has_errors = true;
                    
                }

                g_handlers.clear();

                if (has_errors)
					return std::unexpected(LogError::HandlerRemovalFailed);
                
                return {};
                
            }
            catch (...) {
                return std::unexpected(LogError::Unknown);
            }
        }

        std::expected<void, LogError> set_min_level(LogLevel level) noexcept {
            try {

                g_min_level = level;
                return {};
            }
            catch (...) {
                return std::unexpected(LogError::Unknown);
            }
        }

        std::expected<void, LogError> set_min_level_for_handler(std::string_view handler, LogLevel level) noexcept {
            try {

                auto it = std::find_if(g_handlers.begin(), g_handlers.end(),
                    [handler](const HandlerInfo& h) { return h.name == handler; });

                if (it == g_handlers.end()) 
                    return std::unexpected(LogError::HandlerNotFound);

                it->min_level = level;

                // Would need to recreate handler with new level in Gem::Logger
                // For now, just track it locally

                return {};
            }
            catch (...) {
                return std::unexpected(LogError::Unknown);
            }
        }

        LogLevel get_min_level() noexcept {
            return g_min_level;
        }

        LogStats get_stats() noexcept {
            auto gem_stats = Gem::Logger::instance().get_stats();

            return LogStats{
                .messages_logged = gem_stats.processed_records,
                .messages_dropped = gem_stats.dropped_records,
                .handlers_active = gem_stats.handler_count,
                .messages_per_second = 0.0, // Would need to calculate
                .queue_saturated = gem_stats.queue_saturated
            };
        }

        std::expected<void, LogError> flush() noexcept {
            try {

                auto result = Gem::get_file_cache().flush_all();
                if (result.is_err())
                    return std::unexpected(LogError::Unknown);
                
                return {};
            }
            catch (...) {
                return std::unexpected(LogError::Unknown);
            }
        }

        std::expected<void, LogError> flush_handler(std::string_view handler) noexcept {
            try {

                auto it = std::find_if(g_handlers.begin(), g_handlers.end(),
                    [handler](const HandlerInfo& h) { return h.name == handler; });

                if (it == g_handlers.end()) 
                    return std::unexpected(LogError::HandlerNotFound);

                if (it->is_file) {

                    auto result = Gem::get_file_cache().flush(it->file_path);
                    if (result.is_err()) 
                        return std::unexpected(LogError::Unknown);
                }

                return {};
            }
            catch (...) {
                return std::unexpected(LogError::Unknown);
            }
        }

        // Core logging functions
        void trace(std::string_view msg, const LogContext& ctx, std::source_location loc) noexcept {
            if (!g_initialized.load() || g_min_level > LogLevel::Trace) return;
            try {

                Gem::Logger::trace(msg, to_gem_context(ctx), loc);
            }
            catch (...) {}
        }

        void debug(std::string_view msg, const LogContext& ctx, std::source_location loc) noexcept {
            if (!g_initialized.load() || g_min_level > LogLevel::Debug) return;
            try {

                Gem::Logger::debug(msg, to_gem_context(ctx), loc);
            }
            catch (...) {}
        }

        void info(std::string_view msg, const LogContext& ctx, std::source_location loc) noexcept {
            if (!g_initialized.load() || g_min_level > LogLevel::Info) return;
            try {

                Gem::Logger::info(msg, to_gem_context(ctx), loc);
            }
            catch (...) {}
        }

        void success(std::string_view msg, const LogContext& ctx, std::source_location loc) noexcept {
            if (!g_initialized.load() || g_min_level > LogLevel::Success) return;
            try {

                Gem::Logger::success(msg, to_gem_context(ctx), loc);
            }
            catch (...) {}
        }

        void warning(std::string_view msg, const LogContext& ctx, std::source_location loc) noexcept {
            if (!g_initialized.load() || g_min_level > LogLevel::Warning) return;
            try {

                Gem::Logger::warning(msg, to_gem_context(ctx), loc);
            }
            catch (...) {}
        }

        void error(std::string_view msg, const LogContext& ctx, std::source_location loc) noexcept {
            if (!g_initialized.load() || g_min_level > LogLevel::Error) return;
            try {

                Gem::Logger::error(msg, to_gem_context(ctx), loc);
            }
            catch (...) {}
        }

        void critical(std::string_view msg, const LogContext& ctx, std::source_location loc) noexcept {
            if (!g_initialized.load() || g_min_level > LogLevel::Critical) return;
            try {

                Gem::Logger::critical(msg, to_gem_context(ctx), loc);
            }
            catch (...) {}
        }

        // Utility functions
        std::expected<void, LogError> rotate_file(std::string_view handler) noexcept {
            try {

                auto it = std::find_if(g_handlers.begin(), g_handlers.end(),
                    [handler](const HandlerInfo& h) { return h.name == handler && h.is_file; });

                if (it == g_handlers.end()) 
                    return std::unexpected(LogError::HandlerNotFound);

                // Close and rotate
                auto result = Gem::get_file_cache().close(it->file_path);
                if (result.is_err()) 
                    return std::unexpected(LogError::Unknown);

                return {};
            }
            catch (...) {
                return std::unexpected(LogError::Unknown);
            }
        }

        std::expected<std::size_t, LogError> get_file_size(std::string_view handler) noexcept {
            try {
                
                auto it = std::find_if(g_handlers.begin(), g_handlers.end(),
                    [handler](const HandlerInfo& h) { return h.name == handler && h.is_file; });

                if (it == g_handlers.end()) 
                    return std::unexpected(LogError::HandlerNotFound);
                
                if (std::filesystem::exists(it->file_path)) 
                    return std::filesystem::file_size(it->file_path);
                
                return 0;
            }
            catch (...) {
                return std::unexpected(LogError::Unknown);
            }
        }

        // Advanced configuration
        std::expected<void, LogError> set_queue_size(std::size_t size) noexcept {
            try {

                Gem::Logger::instance().set_queue_capacity(size);
                return {};
            }
            catch (...) {
                return std::unexpected(LogError::Unknown);
            }
        }

        std::expected<void, LogError> set_overflow_policy(bool drop_on_full) noexcept {
            try {

                Gem::Logger::instance().set_overflow_policy(
                    drop_on_full ? Gem::OverflowPolicy::DropNewest : Gem::OverflowPolicy::Block
                );
                return {};
            }
            catch (...) {
                return std::unexpected(LogError::Unknown);
            }
        }

        std::expected<void, LogError> enable_async(bool enable [[maybe_unused]] ) noexcept {
            // This would require switching between Logger and MultiWorkerLogger
            // For now, just return success
            return {};
        }

        // Benchmarking
        std::expected<BenchmarkResult, LogError> benchmark(std::size_t num_messages) noexcept {
            try {

                if (!g_initialized.load()) 
                    return std::unexpected(LogError::NotInitialized);

                auto result = Gem::Logger::instance().benchmark(num_messages);

                return BenchmarkResult{
                    .messages_per_second = result.messages_per_second,
                    .avg_latency = result.avg_latency,
                    .min_latency = result.min_latency,
                    .max_latency = result.max_latency
                };
            }
            catch (...) {
                return std::unexpected(LogError::Unknown);
            }
        }

    } // namespace Logger
} // namespace AshCore