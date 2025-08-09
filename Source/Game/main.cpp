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

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // ==========================================
    // 3. LOGGING WITH CONTEXT DATA
    // ==========================================

    std::cout << "\n--- Logging with Context ---\n";

    // Add context data to your logs
    print_i("User logged in", AshCore::LogContext{
        {"user_id", 12345},
        {"username", "AshCore"},
        {"ip_address", "192.168.1.100"},
        {"login_time", "2024-01-15 10:30:00"}
        });

    print_w("Memory usage high", AshCore::LogContext{
        {"used_mb", 3800},
        {"total_mb", 4096},
        {"percentage", 92.7}
        });

    print_e("Failed to load texture", AshCore::LogContext{
        {"file", "assets/textures/stone.png"},
        {"error_code", 404},
        {"attempted_retries", 3}
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // ==========================================
    // 4. NEW FORMAT SUPPORT
    // ==========================================

    std::cout << "\n--- Format Support Examples ---\n";

    // Simple formatting
    int player_health = 75;
    float player_speed = 5.5f;
    print_i("Player stats: health={}, speed={:.2f}", player_health, player_speed);

    // Multiple variables
    std::string player_name = "AshBorn";
    int level = 42;
    print_i("Player {} reached level {}", player_name, level);

    // Format with context
    int chunk_x = 16, chunk_z = 32;
    print_i("Loading chunk [{}, {}]", chunk_x, chunk_z, AshCore::LogContext{
        {"faces", 15234},
        {"biome", "forest"}
        });

    // ==========================================
    // 5. RUNTIME CONFIGURATION
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
    [[maybe_unused]] auto reset_result = AshCore::Logger::set_min_level(AshCore::LogLevel::Trace);

    // ==========================================
    // 6. PERFORMANCE MONITORING
    // ==========================================

    std::cout << "\n--- Performance Monitoring ---\n";

    // Get current statistics
    auto stats = AshCore::Logger::get_stats();
    print_i("Logger Statistics", AshCore::LogContext{
        {"messages_logged", static_cast<int>(stats.messages_logged)},
        {"messages_dropped", static_cast<int>(stats.messages_dropped)},
        {"handlers_active", static_cast<int>(stats.handlers_active)},
        {"queue_saturated", stats.queue_saturated}
        });

    // ==========================================
    // 7. BENCHMARKING
    // ==========================================

    std::cout << "\n--- Benchmarking ---\n";

    print_i("Starting benchmark with 10,000 messages...");

    auto bench_result = AshCore::Logger::benchmark(10000);
    if (bench_result) {
        print_s("Benchmark completed", AshCore::LogContext{
            {"messages_per_second", bench_result->messages_per_second},
            {"avg_latency_ns", bench_result->avg_latency.count()},
            {"min_latency_ns", bench_result->min_latency.count()},
            {"max_latency_ns", bench_result->max_latency.count()}
            });
    }

    // ==========================================
    // 8. GAME SCENARIOS WITH FORMAT
    // ==========================================

    std::cout << "\n--- Game Scenario Logging ---\n";

    // Simulate chunk loading with format
    for (int i = 0; i < 3; ++i) {
        print_t("Loading chunk at [{}, {}] with {} faces",
            i * 32, 0, 15234 + i * 1000);
    }

    // Combat with formatting
    std::string enemy = "Corrupted Villager";
    int damage = 25;
    print_i("{} dealt {} damage to {}", player_name, damage, enemy);

    // Error with format
    print_e("Failed to save game to '{}': {} MB required, {} MB available",
        "saves/world1.dat", 50, 10);

    // ==========================================
    // 9. MULTI-THREADED LOGGING WITH FORMAT
    // ==========================================

    std::cout << "\n--- Multi-threaded Logging ---\n";

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([i]() {
            for (int j = 0; j < 5; ++j) {
                print_i("Thread {} message {}", i, j);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            });
    }

    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }

    print_s("Multi-threaded logging test completed");

    // ==========================================
    // 10. CLEANUP
    // ==========================================

    std::cout << "\n--- Cleanup ---\n";

    // Get final stats
    stats = AshCore::Logger::get_stats();
    std::cout << "Final message count: " << stats.messages_logged << "\n";

    // Flush everything before shutdown
    [[maybe_unused]] auto flush_result = AshCore::Logger::flush();

    // Shutdown the logger
    auto shutdown_result = AshCore::Logger::shutdown();
    if (shutdown_result) {
        std::cout << "Logger shutdown successfully\n";
    }

    std::cout << "\n=== Example Complete ===\n";

    return 0;
}