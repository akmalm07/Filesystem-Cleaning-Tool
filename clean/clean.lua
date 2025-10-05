project "Clean"
    location "."
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++latest"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    -- Include directories
    includedirs 
    {
        --"%{IncludeDir.item}",
        "global",            
        "include",
        "json",
        "src"
    }

    -- Files
    files 
    {
        "src/**.cpp",           
        "include/**.h",     
        "include/**.inl",     

        "global/**.h",
        "global/**.cpp",

        "json/**.json",

    }

    --[[
    -- Library directories
    libdirs 
    { 
        "%{LibDir.item}",
    }

    -- Links
    links 
    { 
        "item",
    }
    pchheader "headers.h"
    pchsource "headers.cpp"
   
    defines { "ITEM" }
    --]]
    


    -- Toolset and compiler settings
    filter "toolset:msc"
        toolset "msc-v143" --
        buildoptions { "/std:c++23" } 
        
    filter "toolset:gcc or toolset:clang"
        buildoptions { "-std=c++23" }

    -- Configuration settings
    filter "configurations:Debug"
        defines "DEBUG"
        symbols "On"
        optimize "Off"
        runtime "Release"  

    filter "configurations:Release"
        symbols "Off"
        optimize "On"
        defines "NDEBUG"
        runtime "Release"  

    -- Windows system settings
    filter "system:windows"
        systemversion "latest"
        defines "PLATFORM_WINDOWS"
    
    -- Visual Studio specific settings
    filter "action:vs*"
        defines "_CRT_SECURE_NO_WARNINGS"
        staticruntime "on"

    -- Linux and GCC/Clang settings
    filter "system:linux or toolset:gcc or toolset:clang"
        buildoptions { "-include pch.h" }
    
    filter "files:global/headers.cpp"   
        buildoptions { "/Ycheaders.h" }
    