#include "ashbornpch.h"

// Include GLFW before Windows headers to avoid APIENTRY conflict
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#include <GLFW/glfw3.h>

#include "Application.h"
#include "AshbornEngine.h"

#include <algorithm>
#include <numeric>
#include <thread>
#include <iostream>

namespace AshCore {

    // ==========================================
    // CONSTRUCTOR / DESTRUCTOR
    // ==========================================

    Application::Application(const EngineConfig& config)
        : engine_(std::make_unique<AshbornEngine>(config))
        , last_frame_time_(std::chrono::steady_clock::now())
        , current_frame_time_(std::chrono::steady_clock::now()) {

        print_i("Application created with new engine");
    }

    Application::Application(std::unique_ptr<AshbornEngine> engine)
        : engine_(std::move(engine))
        , owns_engine_(true)
        , last_frame_time_(std::chrono::steady_clock::now())
        , current_frame_time_(std::chrono::steady_clock::now()) {

        if (!engine_) {
            print_c("Application created with null engine!");
        }
        print_i("Application created with provided engine");
    }

    Application::~Application() {
        if (running_) {
            print_w("Application destroyed while running - calling shutdown");
            if (callbacks_.on_shutdown) {
                callbacks_.on_shutdown();
            }
        }
        print_i("Application destroyed");
    }

    // ==========================================
    // MAIN RUN FUNCTION
    // ==========================================

    std::expected<void, ApplicationError> Application::run() {
        // Initialize if not already done
        if (!engine_->isInitialized()) {
            if (auto result = initialize(); !result) {
                return result;
            }
        }

        if (running_) {
            return std::unexpected(ApplicationError::AlreadyRunning);
        }

        print_i("Starting application main loop");
        running_ = true;

        // Call start callback
        if (callbacks_.on_start) {
            callbacks_.on_start();
        }

        // Main loop
        while (!shouldClose()) {
            auto frame_result = runFrame();
            if (!frame_result) {
                print_e("Frame execution failed");
                running_ = false;
                return frame_result;
            }
        }

        // Shutdown
        print_i("Exiting application main loop");

        if (callbacks_.on_shutdown) {
            callbacks_.on_shutdown();
        }

        running_ = false;

        // Shutdown engine
        if (auto result = engine_->shutdown(); !result) {
            print_e("Engine shutdown failed");
            return std::unexpected(ApplicationError::Unknown);
        }

        return {};
    }

    // ==========================================
    // SEPARATE CONTROL FUNCTIONS
    // ==========================================

    std::expected<void, ApplicationError> Application::initialize() {
        if (!engine_) {
            print_e("Cannot initialize - engine is null");
            return std::unexpected(ApplicationError::NotInitialized);
        }

        print_i("Initializing application...");

        // Initialize the engine
        if (auto result = engine_->initialize(); !result) {
            print_e("Engine initialization failed");
            return std::unexpected(ApplicationError::EngineInitFailed);
        }

        // Set up GLFW callbacks if we have a window
        if (auto* window = engine_->getWindow()) {
            // Store 'this' pointer for callbacks
            glfwSetWindowUserPointer(window, this);

            // Window focus callback
            glfwSetWindowFocusCallback(window, [](GLFWwindow* w, int focused) {
                auto* app = static_cast<Application*>(glfwGetWindowUserPointer(w));
                if (app && app->callbacks_.on_focus_changed) {
                    app->callbacks_.on_focus_changed(focused == GLFW_TRUE);
                }
                });

            // Window resize callback
            glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int width, int height) {
                auto* app = static_cast<Application*>(glfwGetWindowUserPointer(w));
                if (app && app->callbacks_.on_resize) {
                    app->callbacks_.on_resize(width, height);
                }
                });

            // Key callback for escape to exit
            glfwSetKeyCallback(window, [](GLFWwindow* w, int key, int, int action, int) {
                if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(w));
                    if (app) {
                        app->requestExit();
                    }
                }
                });
        }

        print_s("Application initialized");
        return {};
    }

    std::expected<void, ApplicationError> Application::runFrame() {
        if (!engine_ || !engine_->isInitialized()) {
            return std::unexpected(ApplicationError::NotInitialized);
        }

        // Update timing
        updateTiming();

        // Process window events and input
        processInput();

        // Skip update/render if paused (but still process input)
        if (!paused_) {
            // Fixed timestep for physics
            fixedUpdate();

            // Variable timestep for game logic
            update(timing_.delta_time);

            // Render
            render();

            // GUI overlay
            if (callbacks_.on_gui) {
                callbacks_.on_gui();
            }

            // Present frame
            presentFrame();
        }

        // Frame rate limiting
        limitFrameRate();

        // Update frame count
        timing_.frame_count++;

        return {};
    }

    bool Application::shouldClose() const noexcept {
        if (!running_ || !engine_->isRunning()) {
            return true;
        }

        if (auto* window = engine_->getWindow()) {
            return glfwWindowShouldClose(window);
        }

        return false;
    }

    // ==========================================
    // CONTROL FUNCTIONS
    // ==========================================

    void Application::requestExit() noexcept {
        print_i("Application exit requested");
        running_ = false;
        if (engine_) {
            engine_->requestExit();
        }
        if (auto* window = engine_->getWindow()) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    void Application::setPaused(bool paused) noexcept {
        if (paused_ != paused) {
            paused_ = paused;
            print_i("Application paused state changed", LogContext{ {"paused", paused} });

            if (engine_) {
                engine_->setPaused(paused);
            }
        }
    }

    void Application::setTimeScale(double scale) noexcept {
        time_scale_ = std::max(0.0, scale);
        timing_.time_scale = time_scale_;
        print_d("Time scale set", LogContext{ {"scale", time_scale_} });
    }

    void Application::setTargetFPS(uint32_t fps) noexcept {
        target_fps_ = fps;
        print_i("Target FPS set", LogContext{ {"fps", fps} });
    }

    void Application::setFixedTimestep(double timestep) noexcept {
        fixed_timestep_ = std::max(0.001, timestep);  // Min 1ms
        timing_.fixed_delta_time = fixed_timestep_;
        print_i("Fixed timestep set", LogContext{ {"timestep_ms", fixed_timestep_ * 1000} });
    }

    void Application::setMaxDeltaTime(double max_dt) noexcept {
        max_delta_time_ = std::max(0.001, max_dt);
        print_i("Max delta time set", LogContext{ {"max_dt_ms", max_delta_time_ * 1000} });
    }

    // ==========================================
    // PERFORMANCE QUERIES
    // ==========================================

    double Application::getFPS() const noexcept {
        if (timing_.delta_time > 0.0) {
            return 1.0 / timing_.delta_time;
        }
        return 0.0;
    }

    double Application::getAverageFPS() const noexcept {
        double sum = std::accumulate(
            std::begin(fps_samples_),
            std::end(fps_samples_),
            0.0
        );
        return sum / FPS_SAMPLE_COUNT;
    }

    double Application::getFrameTime() const noexcept {
        return timing_.delta_time * 1000.0;  // Convert to milliseconds
    }

    // ==========================================
    // INTERNAL LOOP FUNCTIONS
    // ==========================================

    void Application::updateTiming() {
        last_frame_time_ = current_frame_time_;
        current_frame_time_ = std::chrono::steady_clock::now();

        // Calculate delta time in seconds
        auto delta_duration = current_frame_time_ - last_frame_time_;
        double raw_delta = std::chrono::duration<double>(delta_duration).count();

        // Apply time scale and clamp to max
        timing_.delta_time = std::min(raw_delta * time_scale_, max_delta_time_);

        // Update total time
        timing_.total_time += timing_.delta_time;

        // Store FPS sample
        fps_samples_[fps_sample_index_] = raw_delta;
        fps_sample_index_ = (fps_sample_index_ + 1) % FPS_SAMPLE_COUNT;
    }

    void Application::processInput() {
        // Poll GLFW events
        glfwPollEvents();

        // Additional input processing would go here
        // (gamepad updates, input mapping, etc.)
    }

    void Application::update(double [[maybe_unused]] dt) {
        if (callbacks_.on_update) {
            callbacks_.on_update(timing_);
        }

        // Update subsystems that need variable timestep
        // (animations, particles, etc.)
    }

    void Application::fixedUpdate() {
        // Accumulate time for fixed timestep
        accumulator_ += timing_.delta_time;

        // Run fixed updates
        int steps = 0;
        while (accumulator_ >= fixed_timestep_) {
            if (callbacks_.on_fixed_update) {
                callbacks_.on_fixed_update(fixed_timestep_);
            }

            // Update physics, collision, etc.

            accumulator_ -= fixed_timestep_;
            steps++;

            // Prevent spiral of death
            if (steps > 5) {
                print_w("Fixed update falling behind - clamping");
                accumulator_ = 0.0;
                break;
            }
        }

        // Calculate interpolation factor for rendering
        timing_.interpolation = accumulator_ / fixed_timestep_;
    }

    void Application::render() {
        if (callbacks_.on_render) {
            callbacks_.on_render(timing_);
        }

        // In real implementation:
        // - Begin frame
        // - Record command buffers
        // - Submit to GPU
    }

    void Application::presentFrame() {
        if (auto* window = engine_->getWindow()) {
            // For Vulkan, this would be vkQueuePresentKHR
            // For now, just swap buffers if using OpenGL
            // glfwSwapBuffers(window);
        }
    }

    void Application::limitFrameRate() {
        if (target_fps_ == 0) return;  // Unlimited

        double target_frame_time = 1.0 / target_fps_;
        auto frame_duration = std::chrono::steady_clock::now() - current_frame_time_;
        double elapsed = std::chrono::duration<double>(frame_duration).count();

        if (elapsed < target_frame_time) {
            // Sleep for remaining time
            double sleep_time = target_frame_time - elapsed;

            // Use a spin-wait for the last millisecond for accuracy
            if (sleep_time > 0.001) {
                std::this_thread::sleep_for(
                    std::chrono::duration<double>(sleep_time - 0.001)
                );
            }

            // Spin-wait for precise timing
            while (elapsed < target_frame_time) {
                frame_duration = std::chrono::steady_clock::now() - current_frame_time_;
                elapsed = std::chrono::duration<double>(frame_duration).count();
            }
        }
    }

    // ==========================================
    // CONVENIENCE FUNCTIONS
    // ==========================================

    int runApplication(const EngineConfig& config, const ApplicationCallbacks& callbacks) {
        try {
            // Initialize logger first
            if (auto result = Logger::init(); !result) {
                std::cerr << "Failed to initialize logger\n";
                return 1;
            }

            print_i("=== AshBorn Starting ===");

            // Create and run application
            Application app(config);
            app.setCallbacks(callbacks);

            if (auto result = app.run(); !result) {
                print_c("Application run failed");
                Logger::shutdown();
                return 1;
            }

            print_i("=== AshBorn Shutdown Complete ===");

            // Shutdown logger
            Logger::shutdown();
            return 0;

        }
        catch (const std::exception& e) {
            print_c("Unhandled exception", LogContext{ {"what", e.what()} });
            return 1;
        }
        catch (...) {
            print_c("Unknown exception");
            return 1;
        }
    }

    int runApplication(const ApplicationCallbacks& callbacks) {
        return runApplication(getDefaultEngineConfig(), callbacks);
    }

} // namespace AshCore