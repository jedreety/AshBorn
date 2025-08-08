-- Source/Engine/Build-Engine.lua
-- Build configuration for AshBorn core engine

project "Engine"
    location( _SCRIPT_DIR )
    targetdir "../../Build/%{cfg.buildcfg}"
    kind "StaticLib"
    language "C++"
    staticruntime "Off"  -- Important for mod compatibility
    
    -- Precompiled header for build performance
    pchheader "ashbornpch.h"
    pchsource "Core/ashbornpch.cpp"
    
    files {
        -- Core systems
        "Core/**.h",
        "Core/**.cpp",
        
        -- Renderer (Vulkan)
        "Renderer/**.h", 
        "Renderer/**.cpp",
        
        -- World management
        "World/**.h",
        "World/**.cpp",
        
        -- Networking
        "Network/**.h",
        "Network/**.cpp",
        
        -- Asset pipeline
        "Asset/**.h",
        "Asset/**.cpp",
        
        -- Audio system
        "Audio/**.h",
        "Audio/**.cpp",
        
        -- Input handling
        "Input/**.h",
        "Input/**.cpp",
        
        -- UI (ImGui integration)
        "UI/**.h",
        "UI/**.cpp",

	"World/**.h",
	"World/**.cpp",
        
        -- Exclude platform-specific files
        "!Core/Platform/**.cpp"
    }
    
    -- Platform-specific source files
    filter "system:windows"
        files {
            "Core/Platform/Windows/**.cpp",
            "Core/Platform/Windows/**.h"
        }
        
    filter "system:linux"
        files {
            "Core/Platform/Linux/**.cpp",
            "Core/Platform/Linux/**.h"
        }
    
    filter {}
    
    includedirs {
        -- Internal includes
        ".",
        "Core",
        "Renderer",
        "World",
        
        -- Dependencies
        "%{IncludeDir.vulkan}",
        "%{IncludeDir.glfw}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.entt}",
        "%{IncludeDir.imgui}",
        "%{IncludeDir.stb}",
        "%{IncludeDir.enet}",
	"%{IncludeDir.gem}"
    }
    
    defines {
        "GLFW_INCLUDE_NONE",  -- We're using Vulkan, not OpenGL
        "GLM_FORCE_RADIANS",
        "GLM_FORCE_DEPTH_ZERO_TO_ONE",  -- Vulkan depth range
        "GLM_ENABLE_EXPERIMENTAL",
        "_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING"  -- For EnTT
    }
    
    -- Vulkan-specific defines
    filter "configurations:Debug"
        defines {
            "VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1",
            "VK_USE_PLATFORM_WIN32_KHR"  -- Platform specific
        }
        
    -- Link dependencies (for static lib, these propagate to consumers)
    links {
        "%{LibraryDir.enet}",
        "%{LibraryDir.glfw}",
	"%{LibraryDir.imgui}"
    }
    
    -- Platform-specific libraries
    filter "system:windows"
        links {
            "%{LibraryDir.vulkan}"
	
        }
        libdirs {
            "$(VULKAN_SDK)/Lib"          -- Where vulkan-1.lib lives
        }

        
    filter "system:linux"
        links {
            "vulkan",
            "dl",
            "pthread",
            "X11",
            "Xxf86vm",
            "Xrandr",
            "Xi"
        }
    
    -- Special handling for mesh shaders (RTX 20XX+ feature)
    filter "configurations:Release or configurations:Dist"
        defines {
            "ASHBORN_ENABLE_MESH_SHADERS"
        }
        
    -- Optimize voxel code aggressively
    filter { "files:World/Voxel/**.cpp", "configurations:Release or configurations:Dist" }
        optimize "Speed"
        vectorextensions "AVX2"  -- Modern CPU optimization
        floatingpoint "Fast"