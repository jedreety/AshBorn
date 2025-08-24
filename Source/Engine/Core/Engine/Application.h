#pragma once

#include <expected>
#include <memory>
#include <functional>
#include <chrono>

namespace AshCore {

    // Forward declarations
    class AshbornEngine;
    struct EngineConfig;

    // ==========================================
    // APPLICATION ERROR TYPES
    // ==========================================

    enum class ApplicationError {
        None = 0,
        EngineInitFailed,
        AlreadyRunning,
        NotInitialized,
        UpdateFailed,
        RenderFailed,
        Unknown
    };

    // ==========================================
    // TIMING INFORMATION
    // ==========================================

    struct FrameTiming {
        double delta_time;           // Time since last frame (seconds)
        double fixed_delta_time;     // Fixed timestep for physics (seconds)
        double time_scale;           // Time multiplier (1.0 = normal speed)
        double total_time;           // Total application time (seconds)
        uint64_t frame_count;        // Total frames rendered
        double interpolation;        // Physics interpolation factor [0,1]
    };

    // ==========================================
    // APPLICATION CALLBACKS
    // ==========================================

    struct ApplicationCallbacks {
        // Called once after engine initialization
        std::function<void()> on_start;

        // Called every frame for game logic (variable timestep)
        std::function<void(const FrameTiming&)> on_update;

        // Called at fixed intervals for physics (fixed timestep)
        std::function<void(double fixed_delta)> on_fixed_update;

        // Called after update for rendering
        std::function<void(const FrameTiming&)> on_render;

        // Called after render for UI overlay
        std::function<void()> on_gui;

        // Called when window loses/gains focus
        std::function<void(bool focused)> on_focus_changed;

        // Called when window is resized
        std::function<void(int width, int height)> on_resize;

        // Called before shutdown
        std::function<void()> on_shutdown;
    };

    // ==========================================
    // MAIN APPLICATION CLASS
    // ==========================================

    class Application {
    public:
        // Constructors
        explicit Application(const EngineConfig& config);
        explicit Application(std::unique_ptr<AshbornEngine> engine);
        ~Application();

        // Delete copy/move
        Application(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&) = delete;

        // Main entry point - blocks until exit
        [[nodiscard]] std::expected<void, ApplicationError> run();

        // Alternative: separate init and loop for more control
        [[nodiscard]] std::expected<void, ApplicationError> initialize();
        [[nodiscard]] std::expected<void, ApplicationError> runFrame();
        [[nodiscard]] bool shouldClose() const noexcept;

        // Control
        void requestExit() noexcept;
        void setPaused(bool paused) noexcept;
        void setTimeScale(double scale) noexcept;
        void setTargetFPS(uint32_t fps) noexcept;
        void setFixedTimestep(double timestep) noexcept;
        void setMaxDeltaTime(double max_dt) noexcept;

        // Callbacks
        void setCallbacks(const ApplicationCallbacks& callbacks) noexcept { callbacks_ = callbacks; }
        ApplicationCallbacks& getCallbacks() noexcept { return callbacks_; }

        // Access
        [[nodiscard]] AshbornEngine* getEngine() noexcept { return engine_.get(); }
        [[nodiscard]] const AshbornEngine* getEngine() const noexcept { return engine_.get(); }
        [[nodiscard]] const FrameTiming& getTiming() const noexcept { return timing_; }

        // Performance
        [[nodiscard]] double getFPS() const noexcept;
        [[nodiscard]] double getAverageFPS() const noexcept;
        [[nodiscard]] double getFrameTime() const noexcept;

    private:
        // Internal loop functions
        void updateTiming();
        void processInput();
        void update(double dt);
        void fixedUpdate();
        void render();
        void presentFrame();

        // Frame limiting
        void limitFrameRate();

    private:
        // Engine
        std::unique_ptr<AshbornEngine> engine_;
        bool owns_engine_ = true;

        // State
        bool running_ = false;
        bool paused_ = false;

        // Timing
        FrameTiming timing_{};
        std::chrono::steady_clock::time_point last_frame_time_;
        std::chrono::steady_clock::time_point current_frame_time_;
        double accumulator_ = 0.0;  // For fixed timestep

        // Settings
        uint32_t target_fps_ = 0;  // 0 = unlimited
        double fixed_timestep_ = 1.0 / 60.0;  // 60 Hz physics
        double max_delta_time_ = 0.25;  // Max 250ms per frame
        double time_scale_ = 1.0;

        // Performance tracking
        static constexpr size_t FPS_SAMPLE_COUNT = 60;
        double fps_samples_[FPS_SAMPLE_COUNT] = {};
        size_t fps_sample_index_ = 0;

        // Callbacks
        ApplicationCallbacks callbacks_;
    };

    // ==========================================
    // CONVENIENCE RUNNER
    // ==========================================

    /**
     * @brief Simple way to run the application with callbacks
     *
     * @param config Engine configuration
     * @param callbacks Application callbacks
     * @return Exit code (0 = success)
     */
    [[nodiscard]] int runApplication(
        const EngineConfig& config,
        const ApplicationCallbacks& callbacks
    );

    /**
     * @brief Run with default config
     */
    [[nodiscard]] int runApplication(const ApplicationCallbacks& callbacks);

} // namespace AshCore