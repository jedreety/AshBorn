#pragma once

#include <expected>
#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <optional>
#include <filesystem>

// Forward declarations for subsystem types
struct GLFWwindow;
struct VkInstance_T;
struct VkDevice_T;

namespace AshCore {

    // ==========================================
    // ERROR DEFINITIONS
    // ==========================================

    enum class EngineError {
        None = 0,
        AlreadyInitialized,
        NotInitialized,
        SubsystemFailure,
        InvalidConfiguration,
        Unknown
    };

    enum class RendererError {
        None = 0,
        VulkanInitFailed,
        NoSuitableGPU,
        SwapchainCreationFailed,
        ValidationLayersUnavailable,
        ExtensionNotSupported,
        ShaderCompilationFailed,
        OutOfGPUMemory,
        Unknown
    };

    enum class WindowError {
        None = 0,
        GLFWInitFailed,
        WindowCreationFailed,
        MonitorNotFound,
        InvalidDimensions,
        SurfaceCreationFailed,
        Unknown
    };

    enum class InputError {
        None = 0,
        InitializationFailed,
        DeviceNotFound,
        MappingFailed,
        Unknown
    };

    enum class AudioError {
        None = 0,
        DeviceInitFailed,
        NoOutputDevice,
        FormatNotSupported,
        BufferCreationFailed,
        Unknown
    };

    enum class WorldError {
        None = 0,
        InitializationFailed,
        InvalidConfiguration,
        ChunkGenerationFailed,
        SerializationFailed,
        Unknown
    };

    enum class NetworkError {
        None = 0,
        InitializationFailed,
        PortBindFailed,
        SteamworksFailed,
        ConnectionFailed,
        Unknown
    };

    enum class AssetError {
        None = 0,
        InitializationFailed,
        PathNotFound,
        LoaderNotFound,
        CorruptedAsset,
        Unknown
    };

    // ==========================================
    // CONFIGURATION STRUCTURES
    // ==========================================

    struct WindowConfig {
        std::string title = "AshBorn";
        int width = 1920;
        int height = 1080;
        bool fullscreen = false;
        bool vsync = true;
        bool resizable = true;
        int monitor_index = 0;  // -1 for windowed, 0+ for specific monitor
        int msaa_samples = 1;   // 1, 2, 4, 8
        bool borderless = false;
    };

    struct RendererConfig {
        bool enable_validation = true;  // Debug layers
        bool enable_mesh_shaders = true;
        bool enable_raytracing = false;  // Future
        bool enable_bindless = true;
        std::vector<const char*> required_extensions;
        std::vector<const char*> optional_extensions;
        uint32_t max_frames_in_flight = 2;
        size_t vram_budget = 0;  // 0 = auto detect
        bool prefer_discrete_gpu = true;
        std::filesystem::path shader_cache_path = "Cache/Shaders";
    };

    struct InputConfig {
        bool raw_mouse_input = true;
        float mouse_sensitivity = 1.0f;
        float controller_deadzone = 0.15f;
        bool enable_haptics = true;
        std::filesystem::path keybind_config = "Config/keybinds.json";
    };

    struct AudioConfig {
        uint32_t sample_rate = 48000;
        uint32_t buffer_size = 512;
        uint8_t channels = 2;
        float master_volume = 1.0f;
        bool enable_3d_audio = true;
        uint32_t max_simultaneous_sounds = 128;
    };

    struct WorldConfig {
        uint32_t chunk_size = 32;
        uint32_t render_distance = 16;  // In chunks
        uint32_t simulation_distance = 8;
        bool enable_lod = true;
        uint32_t max_chunks_per_frame = 4;  // Generation limit
        uint64_t world_seed = 0;  // 0 = random
        std::filesystem::path world_save_path = "Saves/World";
    };

    struct NetworkConfig {
        enum class Mode {
            Offline,
            P2P_Host,
            P2P_Client,
            DedicatedServer,
            DedicatedClient
        };

        Mode mode = Mode::Offline;
        uint16_t port = 7777;
        std::string server_address = "127.0.0.1";
        uint32_t max_players = 4;
        bool use_steam_relay = true;
        uint32_t tick_rate = 60;  // Server tick rate
        uint32_t send_rate = 30;  // Client send rate
    };

    struct AssetConfig {
        std::vector<std::filesystem::path> asset_paths = { "Content" };
        bool enable_hot_reload = true;
        bool validate_assets = true;
        size_t cache_size_mb = 512;
        bool async_loading = true;
        uint32_t loader_threads = 4;
    };

    struct EngineConfig {
        WindowConfig window;
        RendererConfig renderer;
        InputConfig input;
        AudioConfig audio;
        WorldConfig world;
        NetworkConfig network;
        AssetConfig assets;

        // Global settings
        bool enable_profiling = true;
        bool enable_debug_ui = true;
        std::filesystem::path log_path = "Logs";
        uint32_t target_fps = 0;  // 0 = unlimited
    };

    // ==========================================
    // SUBSYSTEM INTERFACES
    // ==========================================

    class ISubsystem {
    public:
        virtual ~ISubsystem() = default;
        virtual const char* getName() const = 0;
        virtual bool isInitialized() const = 0;
    };

    // ==========================================
    // ENGINE STATISTICS
    // ==========================================

    struct EngineStats {
        // Performance
        double fps;
        double frame_time_ms;
        double update_time_ms;
        double render_time_ms;

        // Memory
        size_t ram_used_mb;
        size_t vram_used_mb;
        size_t vram_available_mb;

        // World
        uint32_t chunks_loaded;
        uint32_t entities_active;
        uint32_t faces_rendered;

        // Network
        uint32_t ping_ms;
        uint32_t packets_sent;
        uint32_t packets_received;
        float bandwidth_in_kbps;
        float bandwidth_out_kbps;
    };

    // ==========================================
    // MAIN ENGINE CLASS
    // ==========================================

    class AshbornEngine {
    public:
        // Lifecycle
        explicit AshbornEngine(const EngineConfig& config = {});
        ~AshbornEngine();

        // Delete copy/move to ensure single instance
        AshbornEngine(const AshbornEngine&) = delete;
        AshbornEngine(AshbornEngine&&) = delete;
        AshbornEngine& operator=(const AshbornEngine&) = delete;
        AshbornEngine& operator=(AshbornEngine&&) = delete;

        // Initialization - can be called in stages for flexibility
        [[nodiscard]] std::expected<void, EngineError> initialize();
        [[nodiscard]] std::expected<void, EngineError> initializeCore();     // Logger, Memory
        [[nodiscard]] std::expected<void, RendererError> initializeRenderer();
        [[nodiscard]] std::expected<void, WindowError> initializeWindow();
        [[nodiscard]] std::expected<void, InputError> initializeInput();
        [[nodiscard]] std::expected<void, AudioError> initializeAudio();
        [[nodiscard]] std::expected<void, WorldError> initializeWorld();
        [[nodiscard]] std::expected<void, NetworkError> initializeNetwork();
        [[nodiscard]] std::expected<void, AssetError> initializeAssets();

        // Shutdown - automatic in destructor but can be called manually
        [[nodiscard]] std::expected<void, EngineError> shutdown();
        void shutdownAssets() noexcept;
        void shutdownNetwork() noexcept;
        void shutdownWorld() noexcept;
        void shutdownAudio() noexcept;
        void shutdownInput() noexcept;
        void shutdownWindow() noexcept;
        void shutdownRenderer() noexcept;
        void shutdownCore() noexcept;

        // State queries
        [[nodiscard]] bool isInitialized() const noexcept { return initialized_; }
        [[nodiscard]] bool isRunning() const noexcept { return running_; }
        [[nodiscard]] bool isPaused() const noexcept { return paused_; }

        // Runtime control
        void requestExit() noexcept { running_ = false; }
        void setPaused(bool paused) noexcept { paused_ = paused; }

        // Configuration
        [[nodiscard]] const EngineConfig& getConfig() const noexcept { return config_; }
        [[nodiscard]] std::expected<void, EngineError> updateConfig(const EngineConfig& config);
        [[nodiscard]] std::expected<void, EngineError> reloadConfig(const std::filesystem::path& path);

        // Statistics
        [[nodiscard]] EngineStats getStats() const noexcept;
        [[nodiscard]] double getUptime() const noexcept;

        // Subsystem access (for main loop and game code)
        [[nodiscard]] GLFWwindow* getWindow() const noexcept { return window_; }
        [[nodiscard]] VkDevice_T* getDevice() const noexcept { return device_; }
        [[nodiscard]] VkInstance_T* getInstance() const noexcept { return instance_; }

        // Hot reload support
        [[nodiscard]] std::expected<void, RendererError> reloadShaders();
        [[nodiscard]] std::expected<void, AssetError> reloadAssets();

        // Profiling
        void beginProfile(const std::string& name) noexcept;
        void endProfile(const std::string& name) noexcept;

    private:
        // Internal initialization helpers
        [[nodiscard]] std::expected<void, WindowError> createWindow();
        [[nodiscard]] std::expected<void, RendererError> createVulkanInstance();
        [[nodiscard]] std::expected<void, RendererError> selectPhysicalDevice();
        [[nodiscard]] std::expected<void, RendererError> createLogicalDevice();
        [[nodiscard]] std::expected<void, RendererError> createSwapchain();

        // Subsystem shutdown helpers (noexcept for RAII)
        void cleanupSwapchain() noexcept;
        void cleanupDevice() noexcept;
        void cleanupInstance() noexcept;
        void cleanupWindow() noexcept;

    private:
        // Configuration
        EngineConfig config_;

        // State
        bool initialized_ = false;
        bool running_ = false;
        bool paused_ = false;
        std::chrono::steady_clock::time_point start_time_;

        // Core handles (using raw pointers for C APIs)
        GLFWwindow* window_ = nullptr;
        VkInstance_T* instance_ = nullptr;
        VkDevice_T* device_ = nullptr;

        // Subsystems (when we create them)
        // std::unique_ptr<Renderer> renderer_;
        // std::unique_ptr<World> world_;
        // std::unique_ptr<InputManager> input_;
        // std::unique_ptr<AudioSystem> audio_;
        // std::unique_ptr<NetworkManager> network_;
        // std::unique_ptr<AssetManager> assets_;

        // Statistics tracking
        mutable EngineStats stats_{};
        mutable std::chrono::steady_clock::time_point last_stats_update_;
    };

    // ==========================================
    // UTILITY FUNCTIONS
    // ==========================================

    // Load configuration from file
    [[nodiscard]] std::expected<EngineConfig, EngineError>
        loadEngineConfig(const std::filesystem::path& path);

    // Save configuration to file
    [[nodiscard]] std::expected<void, EngineError>
        saveEngineConfig(const EngineConfig& config, const std::filesystem::path& path);

    // Validate configuration
    [[nodiscard]] std::expected<void, EngineError>
        validateEngineConfig(const EngineConfig& config);

    // Get default configuration based on hardware
    [[nodiscard]] EngineConfig getDefaultEngineConfig();
    [[nodiscard]] EngineConfig getMinimalEngineConfig();  // For tools/tests
    [[nodiscard]] EngineConfig getMaximalEngineConfig(); // Everything enabled

} // namespace AshCore