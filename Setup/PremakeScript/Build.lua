-- premake5.lua
-- Root build configuration for AshBorn voxel engine

-- Helper function to get dependency path
function GetDependencyPath(dep)
    -- Using git submodules approach as documented
    return "%{wks.location}/Dependencies/" .. dep
end

-- Workspace configuration
workspace "AshBorn"
    architecture "x86_64"
    startproject "Game"
    
    configurations {
        "Debug",
        "Release", 
        "Dist"
    }
    
    flags {
        "MultiProcessorCompile",
        "FatalWarnings"  -- Treat warnings as errors for code quality
    }
    
    -- C++23 for modern features
    cppdialect "C++23"
    
    -- Output directories
    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
    
    targetdir ("%{wks.location}/Build/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/Build/Intermediates/" .. outputdir .. "/%{prj.name}")

-- Platform configurations
filter "system:windows"
    systemversion "latest"
    defines { "ASHBORN_PLATFORM_WINDOWS", "_CRT_SECURE_NO_WARNINGS" }
    
    -- MSVC specific options
    buildoptions { 
        "/EHsc",           -- Enable C++ exceptions
        "/Zc:preprocessor", -- Conforming preprocessor
        "/Zc:__cplusplus", -- Correct __cplusplus macro
        "/permissive-",    -- Strict conformance
        "/W4"              -- High warning level
    }

filter "system:linux"
    pic "On"
    systemversion "latest"
    defines { "ASHBORN_PLATFORM_LINUX" }
    buildoptions { "-Wall", "-Wextra", "-pthread" }

-- Configuration settings
filter "configurations:Debug"
    defines { "ASHBORN_DEBUG", "_DEBUG" }
    runtime "Debug"
    symbols "On"
    optimize "Off"
    
    -- Enable debug features
    defines {
        "ASHBORN_ENABLE_ASSERTS",
        "ASHBORN_ENABLE_PROFILING",
        "ASHBORN_ENABLE_VALIDATION_LAYERS"  -- Vulkan validation
    }

filter "configurations:Release"
    defines { "ASHBORN_RELEASE", "NDEBUG" }
    runtime "Release"
    optimize "On"
    symbols "On"  -- Keep symbols for profiling
    
    -- Disable debug overhead but keep diagnostics
    defines {
        "ASHBORN_ENABLE_PROFILING"
    }

filter "configurations:Dist"
    defines { "ASHBORN_DIST", "NDEBUG" }
    runtime "Release"
    optimize "Full"
    symbols "Off"
    
    -- Strip all debug features
    flags { "LinkTimeOptimization" }

-- Reset filters
filter {}

-- Global includes accessible to all projects
IncludeDir = {}
IncludeDir["vulkan"] = GetDependencyPath("glad/include")
IncludeDir["glfw"] = GetDependencyPath("glfw/include")
IncludeDir["glm"] = GetDependencyPath("glm/include")
IncludeDir["entt"] = GetDependencyPath("entt/single_include")
IncludeDir["imgui"] = GetDependencyPath("imgui/include")
IncludeDir["stb"] = GetDependencyPath("stb/include")
IncludeDir["enet"] = GetDependencyPath("enet/include")
IncludeDir["gem"] = GetDependencyPath("gem/include")

-- Library directories
LibraryDir = {}
LibraryDir["enet"] = GetDependencyPath("enet/src")
LibraryDir["glfw"] = GetDependencyPath("glfw/src")
LibraryDir["imgui"] = GetDependencyPath("imgui/src")

-- Groups for project organization    
group "Engine"
    include "Source/Engine/Build-Engine.lua"
    
-- group "Game" 
    -- include "Source/Game/Build-Game.lua"
    
-- group "Tools"
    -- include "Source/ModAPI/Build-ModAPI.lua"
    -- include "Source/Launcher/Build-Launcher.lua"
    -- include "Source/Editor/Build-Editor.lua"
    
-- group "Tests"
    -- include "Tests/Build-Tests.lua"