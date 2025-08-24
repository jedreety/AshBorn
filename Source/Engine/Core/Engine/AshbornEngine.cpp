#include "ashbornpch.h"

// Include GLFW before any Windows headers to avoid APIENTRY conflict
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#include <GLFW/glfw3.h>
#include <glad/vulkan.h>

#include "AshbornEngine.h"

#include <fstream>
#include <thread>

namespace AshCore {

    // ==========================================
    // CONSTRUCTOR / DESTRUCTOR
    // ==========================================

    AshbornEngine::AshbornEngine(const EngineConfig& config)
        : config_(config)
        , start_time_(std::chrono::steady_clock::now())
        , last_stats_update_(std::chrono::steady_clock::now()) {

        print_i("AshbornEngine constructed", LogContext{
            {"window_width", config.window.width},
            {"window_height", config.window.height},
            {"render_distance", config.world.render_distance}
            });
    }

    AshbornEngine::~AshbornEngine() {
        // Ensure clean shutdown even if not explicitly called
        if (initialized_) {
            print_w("AshbornEngine destructor calling shutdown - should be done explicitly!");
            auto result = shutdown();
            if (!result) {
                print_e("Failed to shutdown engine in destructor");
            }
        }
        print_i("AshbornEngine destroyed");
    }

    // ==========================================
    // MAIN INITIALIZATION
    // ==========================================

    std::expected<void, EngineError> AshbornEngine::initialize() {
        if (initialized_) {
            return std::unexpected(EngineError::AlreadyInitialized);
        }

        print_i("Starting AshbornEngine initialization sequence...");

        // Validate configuration first
        if (auto result = validateEngineConfig(config_); !result) {
            print_e("Invalid engine configuration");
            return std::unexpected(EngineError::InvalidConfiguration);
        }

        // Initialize core systems (Logger is already initialized in main)
        if (auto result = initializeCore(); !result) {
            print_c("Core initialization failed");
            return std::unexpected(EngineError::SubsystemFailure);
        }

        // Initialize window (must be before renderer for surface creation)
        if (auto result = initializeWindow(); !result) {
            print_c("Window initialization failed");
            shutdownCore();
            return std::unexpected(EngineError::SubsystemFailure);
        }

        // Initialize renderer
        if (auto result = initializeRenderer(); !result) {
            print_c("Renderer initialization failed");
            shutdownWindow();
            shutdownCore();
            return std::unexpected(EngineError::SubsystemFailure);
        }

        // Initialize input
        if (auto result = initializeInput(); !result) {
            print_e("Input initialization failed");
            shutdownRenderer();
            shutdownWindow();
            shutdownCore();
            return std::unexpected(EngineError::SubsystemFailure);
        }

        // Initialize audio
        if (auto result = initializeAudio(); !result) {
            print_w("Audio initialization failed - continuing without audio");
            // Non-critical, continue
        }

        // Initialize world
        if (auto result = initializeWorld(); !result) {
            print_e("World initialization failed");
            shutdownAudio();
            shutdownInput();
            shutdownRenderer();
            shutdownWindow();
            shutdownCore();
            return std::unexpected(EngineError::SubsystemFailure);
        }

        // Initialize network (based on config)
        if (config_.network.mode != NetworkConfig::Mode::Offline) {
            if (auto result = initializeNetwork(); !result) {
                print_w("Network initialization failed - falling back to offline mode");
                config_.network.mode = NetworkConfig::Mode::Offline;
                // Non-critical for single player
            }
        }

        // Initialize asset system
        if (auto result = initializeAssets(); !result) {
            print_e("Asset system initialization failed");
            shutdownNetwork();
            shutdownWorld();
            shutdownAudio();
            shutdownInput();
            shutdownRenderer();
            shutdownWindow();
            shutdownCore();
            return std::unexpected(EngineError::SubsystemFailure);
        }

        initialized_ = true;
        running_ = true;

        print_s("AshbornEngine initialization complete", LogContext{
            {"uptime_ms", getUptime() * 1000}
            });

        return {};
    }

    // ==========================================
    // SUBSYSTEM INITIALIZATION
    // ==========================================

    std::expected<void, EngineError> AshbornEngine::initializeCore() {
        print_d("Initializing core systems...");

        // Memory allocators would go here
        // Thread pool initialization
        // Performance counters

        print_s("Core systems initialized");
        return {};
    }

    std::expected<void, WindowError> AshbornEngine::initializeWindow() {
        print_d("Initializing window system...");

        // Initialize GLFW
        if (!glfwInit()) {
            print_e("Failed to initialize GLFW");
            return std::unexpected(WindowError::GLFWInitFailed);
        }

        // Create window
        if (auto result = createWindow(); !result) {
            glfwTerminate();
            return result;
        }

        print_s("Window system initialized", LogContext{
            {"width", config_.window.width},
            {"height", config_.window.height},
            {"fullscreen", config_.window.fullscreen}
            });

        return {};
    }

    std::expected<void, RendererError> AshbornEngine::initializeRenderer() {
        print_d("Initializing Vulkan renderer...");

        // Create Vulkan instance
        if (auto result = createVulkanInstance(); !result) {
            return result;
        }

        // Select physical device
        if (auto result = selectPhysicalDevice(); !result) {
            cleanupInstance();
            return result;
        }

        // Create logical device
        if (auto result = createLogicalDevice(); !result) {
            cleanupInstance();
            return result;
        }

        // Create swapchain
        if (auto result = createSwapchain(); !result) {
            cleanupDevice();
            cleanupInstance();
            return result;
        }

        print_s("Vulkan renderer initialized");
        return {};
    }

    std::expected<void, InputError> AshbornEngine::initializeInput() {
        print_d("Initializing input system...");

        // Set up GLFW callbacks
        // Initialize gamepad support
        // Load keybind configuration

        print_s("Input system initialized");
        return {};
    }

    std::expected<void, AudioError> AshbornEngine::initializeAudio() {
        print_d("Initializing audio system...");

        // Initialize audio backend (OpenAL/FMOD/etc)
        // Create audio device
        // Set up 3D audio listener

        print_s("Audio system initialized");
        return {};
    }

    std::expected<void, WorldError> AshbornEngine::initializeWorld() {
        print_d("Initializing world system...");

        // Initialize ECS (EnTT)
        // Set up chunk manager
        // Initialize physics
        // Load or generate world

        print_s("World system initialized", LogContext{
            {"chunk_size", config_.world.chunk_size},
            {"render_distance", config_.world.render_distance}
            });

        return {};
    }

    std::expected<void, NetworkError> AshbornEngine::initializeNetwork() {
        print_d("Initializing network system...");

        // Initialize networking library (ENet)
        // Set up Steam integration if enabled
        // Create server or connect to server based on mode

        print_s("Network system initialized", LogContext{
            {"mode", static_cast<int>(config_.network.mode)},
            {"port", config_.network.port}
            });

        return {};
    }

    std::expected<void, AssetError> AshbornEngine::initializeAssets() {
        print_d("Initializing asset system...");

        // Validate asset paths
        for (const auto& path : config_.assets.asset_paths) {
            if (!std::filesystem::exists(path)) {
                print_e("Asset path not found", LogContext{ {"path", path.string()} });
                return std::unexpected(AssetError::PathNotFound);
            }
        }

        // Initialize asset loaders
        // Set up cache
        // Start loader threads if async

        print_s("Asset system initialized", LogContext{
            {"paths", config_.assets.asset_paths.size()},
            {"cache_mb", config_.assets.cache_size_mb}
            });

        return {};
    }

    // ==========================================
    // SHUTDOWN
    // ==========================================

    std::expected<void, EngineError> AshbornEngine::shutdown() {
        if (!initialized_) {
            return std::unexpected(EngineError::NotInitialized);
        }

        print_i("Starting AshbornEngine shutdown sequence...");

        running_ = false;

        // Shutdown in reverse order
        shutdownAssets();
        shutdownNetwork();
        shutdownWorld();
        shutdownAudio();
        shutdownInput();
        shutdownRenderer();
        shutdownWindow();
        shutdownCore();

        initialized_ = false;

        print_s("AshbornEngine shutdown complete");
        return {};
    }

    void AshbornEngine::shutdownAssets() noexcept {
        print_d("Shutting down asset system...");
        // Clean up asset manager
    }

    void AshbornEngine::shutdownNetwork() noexcept {
        print_d("Shutting down network system...");
        // Disconnect from server
        // Clean up network resources
    }

    void AshbornEngine::shutdownWorld() noexcept {
        print_d("Shutting down world system...");
        // Save world
        // Clean up chunks
        // Destroy ECS
    }

    void AshbornEngine::shutdownAudio() noexcept {
        print_d("Shutting down audio system...");
        // Stop all sounds
        // Destroy audio device
    }

    void AshbornEngine::shutdownInput() noexcept {
        print_d("Shutting down input system...");
        // Remove callbacks
        // Save keybinds
    }

    void AshbornEngine::shutdownRenderer() noexcept {
        print_d("Shutting down renderer...");
        cleanupSwapchain();
        cleanupDevice();
        cleanupInstance();
    }

    void AshbornEngine::shutdownWindow() noexcept {
        print_d("Shutting down window system...");
        cleanupWindow();
        glfwTerminate();
    }

    void AshbornEngine::shutdownCore() noexcept {
        print_d("Shutting down core systems...");
        // Clean up thread pool
        // Clean up memory allocators
    }

    // ==========================================
    // INTERNAL HELPERS
    // ==========================================

    std::expected<void, WindowError> AshbornEngine::createWindow() {
        // Configure GLFW for Vulkan (no OpenGL context)
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, config_.window.resizable ? GLFW_TRUE : GLFW_FALSE);

        // Determine monitor for fullscreen
        GLFWmonitor* monitor = nullptr;
        if (config_.window.fullscreen) {
            int monitor_count;
            GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);

            if (config_.window.monitor_index >= 0 && config_.window.monitor_index < monitor_count) {
                monitor = monitors[config_.window.monitor_index];
            }
            else {
                monitor = glfwGetPrimaryMonitor();
            }
        }

        // Create window
        window_ = glfwCreateWindow(
            config_.window.width,
            config_.window.height,
            config_.window.title.c_str(),
            monitor,
            nullptr
        );

        if (!window_) {
            print_e("Failed to create GLFW window");
            return std::unexpected(WindowError::WindowCreationFailed);
        }

        // Center window if windowed
        if (!config_.window.fullscreen && !monitor) {
            GLFWmonitor* primary = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(primary);
            int pos_x = (mode->width - config_.window.width) / 2;
            int pos_y = (mode->height - config_.window.height) / 2;
            glfwSetWindowPos(window_, pos_x, pos_y);
        }

        return {};
    }

    std::expected<void, RendererError> AshbornEngine::createVulkanInstance() {
        // This is placeholder - actual Vulkan initialization is complex
        print_d("Creating Vulkan instance...");

        // In real implementation:
        // - Check validation layer availability
        // - Get required extensions from GLFW
        // - Create VkInstance
        // - Set up debug messenger if validation enabled

        return {};
    }

    std::expected<void, RendererError> AshbornEngine::selectPhysicalDevice() {
        print_d("Selecting physical device...");

        // In real implementation:
        // - Enumerate physical devices
        // - Score them based on features and performance
        // - Select best one (prefer discrete GPU)

        return {};
    }

    std::expected<void, RendererError> AshbornEngine::createLogicalDevice() {
        print_d("Creating logical device...");

        // In real implementation:
        // - Set up queue families
        // - Enable required features (mesh shaders!)
        // - Create VkDevice

        return {};
    }

    std::expected<void, RendererError> AshbornEngine::createSwapchain() {
        print_d("Creating swapchain...");

        // In real implementation:
        // - Query surface capabilities
        // - Choose format, present mode, extent
        // - Create swapchain
        // - Get swapchain images

        return {};
    }

    void AshbornEngine::cleanupSwapchain() noexcept {
        // Destroy swapchain resources
    }

    void AshbornEngine::cleanupDevice() noexcept {
        // Destroy logical device
        if (device_) {
            // vkDestroyDevice(device_, nullptr);
            device_ = nullptr;
        }
    }

    void AshbornEngine::cleanupInstance() noexcept {
        // Destroy Vulkan instance
        if (instance_) {
            // vkDestroyInstance(instance_, nullptr);
            instance_ = nullptr;
        }
    }

    void AshbornEngine::cleanupWindow() noexcept {
        if (window_) {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
    }

    // ==========================================
    // RUNTIME FUNCTIONS
    // ==========================================

    std::expected<void, EngineError> AshbornEngine::updateConfig(const EngineConfig& config) {
        if (!initialized_) {
            config_ = config;
            return {};
        }

        // Hot-reload what we can
        // Some changes may require restart

        print_w("Runtime config update not fully implemented");
        return {};
    }

    std::expected<void, EngineError> AshbornEngine::reloadConfig(const std::filesystem::path& path) {
        auto config_result = loadEngineConfig(path);
        if (!config_result) {
            return std::unexpected(config_result.error());
        }

        return updateConfig(*config_result);
    }

    std::expected<void, RendererError> AshbornEngine::reloadShaders() {
        print_i("Reloading shaders...");

        // In real implementation:
        // - Wait for GPU idle
        // - Recompile shaders
        // - Recreate pipelines

        print_s("Shaders reloaded");
        return {};
    }

    std::expected<void, AssetError> AshbornEngine::reloadAssets() {
        print_i("Reloading assets...");

        // Flush cache
        // Reload modified assets

        print_s("Assets reloaded");
        return {};
    }

    // ==========================================
    // STATISTICS
    // ==========================================

    EngineStats AshbornEngine::getStats() const noexcept {
        // Update stats if stale
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_stats_update_).count();

        if (elapsed > 100) {  // Update every 100ms
            // Gather fresh stats
            last_stats_update_ = now;

            // In real implementation, query all subsystems
            stats_.fps = 60.0;  // Placeholder
            stats_.frame_time_ms = 16.67;
            // etc...
        }

        return stats_;
    }

    double AshbornEngine::getUptime() const noexcept {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration<double>(now - start_time_);
        return duration.count();
    }

    // ==========================================
    // PROFILING
    // ==========================================

    void AshbornEngine::beginProfile(const std::string& name) noexcept {
        if (!config_.enable_profiling) return;

        // Start timing
        print_t("Profile begin", LogContext{ {"name", name} });
    }

    void AshbornEngine::endProfile(const std::string& name) noexcept {
        if (!config_.enable_profiling) return;

        // End timing and record
        print_t("Profile end", LogContext{ {"name", name} });
    }

    // ==========================================
    // UTILITY FUNCTIONS
    // ==========================================

    std::expected<EngineConfig, EngineError> loadEngineConfig(const std::filesystem::path& path) {
        if (!std::filesystem::exists(path)) {
            print_e("Config file not found", LogContext{ {"path", path.string()} });
            return std::unexpected(EngineError::InvalidConfiguration);
        }

        // In real implementation: parse JSON/TOML config file

        return getDefaultEngineConfig();
    }

    std::expected<void, EngineError> saveEngineConfig(const EngineConfig& config, const std::filesystem::path& path) {
        try {
            // Create directory if needed
            std::filesystem::create_directories(path.parent_path());

            // In real implementation: serialize to JSON/TOML
            std::ofstream file(path);
            if (!file) {
                return std::unexpected(EngineError::Unknown);
            }

            // Write config...

            print_s("Config saved", LogContext{ {"path", path.string()} });
            return {};
        }
        catch (...) {
            return std::unexpected(EngineError::Unknown);
        }
    }

    std::expected<void, EngineError> validateEngineConfig(const EngineConfig& config) {
        // Validate window dimensions
        if (config.window.width <= 0 || config.window.height <= 0) {
            print_e("Invalid window dimensions");
            return std::unexpected(EngineError::InvalidConfiguration);
        }

        // Validate chunk size (must be power of 2)
        if (config.world.chunk_size == 0 || (config.world.chunk_size & (config.world.chunk_size - 1)) != 0) {
            print_e("Chunk size must be power of 2");
            return std::unexpected(EngineError::InvalidConfiguration);
        }

        // More validation...

        return {};
    }

    EngineConfig getDefaultEngineConfig() {
        EngineConfig config;

        // Detect hardware and set appropriate defaults
        unsigned int thread_count = std::thread::hardware_concurrency();
        config.assets.loader_threads = std::min(thread_count / 2, 4u);

        // Check available RAM
        // Check GPU capabilities
        // etc...

        return config;
    }

    EngineConfig getMinimalEngineConfig() {
        EngineConfig config;

        // Minimal for tests/tools
        config.window.width = 800;
        config.window.height = 600;
        config.renderer.enable_validation = false;
        config.renderer.enable_mesh_shaders = false;
        config.world.render_distance = 4;
        config.assets.async_loading = false;
        config.network.mode = NetworkConfig::Mode::Offline;

        return config;
    }

    EngineConfig getMaximalEngineConfig() {
        EngineConfig config;

        // Everything enabled for showcase
        config.window.width = 3840;
        config.window.height = 2160;
        config.renderer.enable_validation = true;
        config.renderer.enable_mesh_shaders = true;
        config.renderer.enable_raytracing = true;
        config.renderer.enable_bindless = true;
        config.world.render_distance = 32;
        config.assets.cache_size_mb = 2048;

        return config;
    }

} // namespace AshCore