#include <windows.h>
#include <sal.h> // Ajoute les annotations SAL

#include <Core/Logger/log.h>
#include <thread>
#include <chrono>
#include <iostream>

int  main()
{

    // ==========================================
    // 1. INITIALIZATION
    // ==========================================

    std::cout << "=== AshCore Logger Example ===\n\n";

    // Initialize the logger (required before any logging)
    auto init_result = AshCore::Logger::init();
    if (!init_result) {
        std::cerr << "Failed to initialize logger!\n";
        return 1;
    }

    std::cout << "Logger initialized successfully!\n\n";

    // ==========================================
    // 2. BASIC LOGGING (Using Macros)
    // ==========================================

    std::cout << "--- Basic Logging ---\n";

    // Simple messages
    print_t("This is a trace message - very detailed debug info");
    print_d("This is a debug message");
    print_i("This is an info message");
    print_s("This is a success message - something went well!");
    print_w("This is a warning message");
    print_e("This is an error message");
    print_c("This is a critical message - something is very wrong!");

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Let messages print

    // ==========================================
    // 3. LOGGING WITH CONTEXT DATA
    // ==========================================

    std::cout << "\n--- Logging with Context ---\n";

    // Add context data to your logs
    print_i("User logged in", {
        {"user_id", 12345},
        {"username", "AshCore"},
        {"ip_address", "192.168.1.100"},
        {"login_time", "2024-01-15 10:30:00"}
        });

    print_w("Memory usage high", {
        {"used_mb", 3800},
        {"total_mb", 4096},
        {"percentage", 92.7}
        });

    print_e("Failed to load texture", {
        {"file", "assets/textures/stone.png"},
        {"error_code", 404},
        {"attempted_retries", 3}
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

   
    // ==========================================
    // 6. RUNTIME CONFIGURATION
    // ==========================================

    std::cout << "\n--- Runtime Configuration ---\n";

    // Change minimum log level at runtime
    auto level_result = AshCore::Logger::set_min_level(AshCore::LogLevel::Warning);
    if (level_result) {
        std::cout << "Changed minimum log level to Warning\n";
    }

    // These won't be logged anymore (below Warning)
    print_t("This trace won't show");
    print_d("This debug won't show");
    print_i("This info won't show");

    // These will still be logged
    print_w("This warning will show");
    print_e("This error will show");

    // Reset to Trace for rest of demo
    auto r = AshCore::Logger::set_min_level(AshCore::LogLevel::Trace);

    // ==========================================
    // 7. PERFORMANCE MONITORING
    // ==========================================

    std::cout << "\n--- Performance Monitoring ---\n";

    // Get current statistics
    auto stats = AshCore::Logger::get_stats();
    print_i("Logger Statistics", {
        {"messages_logged", static_cast<int>(stats.messages_logged)},
        {"messages_dropped", static_cast<int>(stats.messages_dropped)},
        {"handlers_active", static_cast<int>(stats.handlers_active)},
        {"queue_saturated", stats.queue_saturated}
        });

    // ==========================================
    // 8. BENCHMARKING
    // ==========================================

    std::cout << "\n--- Benchmarking ---\n";

    print_i("Starting benchmark with 10,000 messages...");

    auto bench_result = AshCore::Logger::benchmark(10000);
    if (bench_result) {
        print_s("Benchmark completed", {
            {"messages_per_second", bench_result->messages_per_second},
            {"avg_latency_ns", bench_result->avg_latency.count()},
            {"min_latency_ns", bench_result->min_latency.count()},
            {"max_latency_ns", bench_result->max_latency.count()}
            });
    }

    // ==========================================
    // 9. ADVANCED FEATURES
    // ==========================================

    std::cout << "\n--- Advanced Features ---\n";

    // Configure queue behavior
    auto re = AshCore::Logger::set_queue_size(16384);  // Increase queue size
    auto res = AshCore::Logger::set_overflow_policy(true);  // Drop messages if queue full

    print_i("Queue configured for high-throughput");

    // Flush all pending logs
    auto flush_result = AshCore::Logger::flush();
    if (flush_result) {
        print_s("All logs flushed to disk");
    }

    // Check file size
    auto size_result = AshCore::Logger::get_file_size("game_log");
    if (size_result) {
        print_i("Log file size", { {"bytes", static_cast<int>(*size_result)} });
    }

    // ==========================================
    // 10. SIMULATING GAME SCENARIOS
    // ==========================================

    std::cout << "\n--- Game Scenario Logging ---\n";

    // Simulate game initialization
    print_i("Initializing game engine...");
    print_d("Loading configuration from config.json");
    print_s("Configuration loaded successfully");

    print_i("Initializing Vulkan renderer...");
    print_d("Selected GPU: NVIDIA RTX 3050 Ti");
    print_d("Vulkan version: 1.3.275");
    print_s("Vulkan initialized successfully");

    // Simulate chunk loading
    for (int i = 0; i < 3; ++i) {
        print_t("Loading chunk", {
            {"chunk_x", i * 32},
            {"chunk_z", 0},
            {"faces", 15234 + i * 1000}
            });
    }

    // Simulate player actions
    print_i("Player spawned", {
        {"position_x", 128.5},
        {"position_y", 64.0},
        {"position_z", 256.5},
        {"health", 100},
        {"world", "overworld"}
        });

    // Simulate combat
    print_i("Combat started", { {"enemy", "Corrupted Villager"} });
    print_d("Calculating damage", { {"base_damage", 10}, {"armor", 5} });
    print_w("Player health low", { {"health", 20} });
    print_c("Player died", { {"killer", "Corrupted Villager"}, {"death_count", 1} });

    // Simulate error scenarios
    print_e("Failed to save game", {
        {"error", "Disk full"},
        {"save_path", "saves/world1.dat"},
        {"required_space_mb", 50},
        {"available_space_mb", 10}
        });

    // ==========================================
    // 11. MULTI-THREADED LOGGING
    // ==========================================

    std::cout << "\n--- Multi-threaded Logging ---\n";

    // Create multiple threads that log simultaneously
    std::vector<std::thread> threads;

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([i]() {
            for (int j = 0; j < 5; ++j) {
                print_i("Thread message", {
                    {"thread_id", i},
                    {"message_num", j},
                    {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
                    });
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            });
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    print_s("Multi-threaded logging test completed");

    // ==========================================
    // 12. CLEANUP
    // ==========================================

    std::cout << "\n--- Cleanup ---\n";

    // Get final stats
    stats = AshCore::Logger::get_stats();
    std::cout << "Final message count: " << stats.messages_logged << "\n";

    // Remove specific handler
    auto remove_result = AshCore::Logger::remove_handler("json_log");
    if (remove_result) {
        print_i("Removed JSON handler");
    }

    // Flush everything before shutdown
    auto resu = AshCore::Logger::flush();

    // Shutdown the logger (optional - will happen automatically)
    auto shutdown_result = AshCore::Logger::shutdown();
    if (shutdown_result) {
        std::cout << "Logger shutdown successfully\n";
    }

    std::cout << "\n=== Example Complete ===\n";

    return 0;
}