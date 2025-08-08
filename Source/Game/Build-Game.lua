-- Source/Game/Build-Game.lua
-- Build configuration for AshBorn game executable

project "Game"
    location( _SCRIPT_DIR )
    targetdir "../../Build/%{cfg.buildcfg}"
    kind "WindowedApp"  -- No console window
    language "C++"
    staticruntime "Off"
    
    files {
        "**.h",
        "**.cpp",
        
        -- Resources
        "**.rc",
        "**.ico"
    }
    
    includedirs {
        ".",
        
        -- Engine access
        "../Engine",
        "../Engine/Core",
        "../Engine/Renderer",
        "../Engine/World",
        
        -- Mod API interfaces
        "../ModAPI/Interfaces",
        
        -- Dependencies
        "%{IncludeDir.glm}",
        "%{IncludeDir.entt}",
        "%{IncludeDir.imgui}"
    }
    
    links {
        "Engine"
        -- "ModAPI"
    }
    
    defines {
        "ASHBORN_GAME"
    }
    
    -- Copy game content to output
        -- Platform specific settings
    filter "system:windows"
        
        links {
            "dwmapi",  -- For window effects
            "winmm"    -- For high-resolution timers
        }