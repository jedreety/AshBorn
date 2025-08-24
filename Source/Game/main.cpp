#include <Core/Engine/Application.h>
#include <Core/Engine/AshbornEngine.h>
#include <Core/Logger/log.h>
#include <iostream>

using namespace AshCore;

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    // Option 1: Simple approach with callbacks
    return runApplication({
        .on_start = []() {
            print_i("Game started!");
        },

        .on_update = [](const FrameTiming& timing) {
            // Game logic here
            if (static_cast<int>(timing.total_time) % 5 == 0) {
                print_d("Update", LogContext{
                    {"fps", 1.0 / timing.delta_time},
                    {"frame", timing.frame_count}
                });
            }
        },

        .on_fixed_update = []([[maybe_unused]] double dt) {
            // Physics update at fixed 60Hz
        },

        .on_render = []([[maybe_unused]] const FrameTiming& timing) {
            // Render scene with interpolation
            // Use timing.interpolation for smooth movement
        },

        .on_gui = []() {
            // ImGui rendering would go here
        },

        .on_resize = [](int width, int height) {
            print_i("Window resized", LogContext{
                {"width", width},
                {"height", height}
            });
        },

        .on_shutdown = []() {
            print_i("Game shutting down - saving progress...");
        }
        });
}

// Option 2: More control with manual setup
int main_with_control([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    // Initialize logger
    if (auto result = Logger::init(); !result) {
        std::cerr << "Logger init failed\n";
        return 1;
    }

    print_i("=== AshBorn Starting ===");

    // Create custom config
    EngineConfig config = getDefaultEngineConfig();
    config.window.title = "AshBorn - Development Build";
    config.window.width = 1920;
    config.window.height = 1080;
    config.renderer.enable_validation = true;
    config.renderer.enable_mesh_shaders = true;
    config.world.render_distance = 16;
    config.network.mode = NetworkConfig::Mode::Offline;

    // Create engine and application
    auto engine = std::make_unique<AshbornEngine>(config);
    Application app(std::move(engine));

    // Set up callbacks
    app.getCallbacks().on_update = [&app](const FrameTiming& timing) {
        // Access engine directly
        auto* engine = app.getEngine();
        auto stats = engine->getStats();

        // Game logic
        static double last_report = 0.0;
        if (timing.total_time - last_report > 1.0) {
            print_i("Performance", LogContext{
                {"fps", stats.fps},
                {"chunks", stats.chunks_loaded},
                {"faces", stats.faces_rendered}
                });
            last_report = timing.total_time;
        }

        // Check for reload keys
        if (/* F5 pressed */ false) {
            auto reload_result = engine->reloadShaders();
            if (!reload_result) {
                print_e("Shader reload failed");
            }
        }
        };

    // Initialize
    if (auto result = app.initialize(); !result) {
        print_e("App initialization failed");
        Logger::shutdown();
        return 1;
    }

    // Custom main loop with more control
    while (!app.shouldClose()) {
        // Could add custom pre-frame logic here

        if (auto result = app.runFrame(); !result) {
            print_e("Frame failed");
            break;
        }

        // Could add custom post-frame logic here
    }

    // Manual shutdown
    auto shutdown_result = app.getEngine()->shutdown();
    if (!shutdown_result) {
        print_e("Engine shutdown failed");
    }

    print_i("=== AshBorn Shutdown Complete ===");
    auto logger_shutdown = Logger::shutdown();
    if (!logger_shutdown) {
        std::cerr << "Logger shutdown failed\n";
    }

    return 0;
}

// Option 3: Minimal for testing
int main_minimal([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    return runApplication(
        getMinimalEngineConfig(),
        {
            .on_update = []([[maybe_unused]] const FrameTiming& timing) {
            // Minimal test logic
        }
        }
    );
}