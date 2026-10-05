workspace "cod4rad"
    location "build"
    configurations { "Debug", "Release" }
    platforms { "Win32", "x64" }
    startproject "cod4rad"

    if _ACTION and not _ACTION:startswith("vs") then
        error("cod4rad requires MSVC.  The original is an x87 MSVC build and other compilers change floating-point results.")
    end

project "cod4rad"
    kind "ConsoleApp"
    language "C++"
    targetdir "bin"
    objdir "build/obj/%{cfg.platform}/%{cfg.buildcfg}"
    targetname "cod4rad"

    -- Original is built with the static, non-debug CRT in every configuration
    staticruntime "On"
    runtime "Release"

    files {
        "cod4rad/**.cpp", "cod4rad/**.h",
        "cod4map/materials.cpp", "cod4map/materials.h",
        "common/**.cpp", "common/**.h",
        "libs/cmdlib/**.cpp", "libs/cmdlib/**.h",
        "src/universal/**.cpp", "src/universal/**.h",
        "src/physics/ode/src/mass.cpp", "src/physics/ode/src/matrix.cpp",
        "src/physics/ode/src/**.h",
        "src/script/**.cpp", "src/script/**.h",
        "src/qcommon/**.cpp", "src/qcommon/**.h",
        "src/gfx_d3d/**.cpp", "src/gfx_d3d/**.h",
        "src/xanim/**.cpp", "src/xanim/**.h",
        "src/zlib/**.c", "src/zlib/**.h",
    }

    includedirs {
        "cod4rad",
        "cod4map",
        "common",
        "libs/cmdlib",
        "src",
        "src/universal",
        "src/qcommon",
        "src/gfx_d3d",
        "src/xanim",
        "src/script",
        "src/zlib",
    }

    vpaths {
        ["cod4rad/*"]        = { "cod4rad/**" },
        ["common/*"]         = { "common/**" },
        ["cod4map/*"]        = { "cod4map/**" },
        ["libs/cmdlib/*"]    = { "libs/cmdlib/**" },
        ["src/universal/*"]  = { "src/universal/**" },
        ["src/physics/*"]    = { "src/physics/**" },
        ["src/script/*"]     = { "src/script/**" },
        ["src/qcommon/*"]    = { "src/qcommon/**" },
        ["src/gfx_d3d/*"]    = { "src/gfx_d3d/**" },
        ["src/xanim/*"]      = { "src/xanim/**" },
        ["src/zlib/*"]       = { "src/zlib/**" },
    }

    defines { "WIN32", "_WINDOWS", "_CRT_SECURE_NO_WARNINGS" }

    -- Win32 is the byte-exact build: x87 codegen on purpose, since SSE2 changes
    -- floating-point results and output bytes.  x64 always uses SSE2 and is not
    -- byte-exact; it exists for maps that run out of 32-bit address space.
    filter "platforms:Win32"
        architecture "x86"
        vectorextensions "IA32"   -- /arch:IA32

    filter "platforms:x64"
        architecture "x86_64"
        targetdir "bin/x64"
        linkoptions { "/STACK:0x1000000" }   -- frames are bigger with 8 byte pointers
        buildoptions { "/w14311", "/w14312", "/w14302", "/w14306" }   -- pointer <-> int truncation

    filter {}
    floatingpoint "Default"
    buildoptions { "/fp:precise", "/GS-", "/Gy" }
    warnings "Default"            -- /W3
    disablewarnings { "4996", "4244", "4018", "4267", "4305", "4101", "4700" }
    exceptionhandling "On"        -- /EHsc
    editandcontinue "Off"

    linkoptions {
        "/SUBSYSTEM:CONSOLE",
    }

    filter "platforms:Win32"
        linkoptions { "/STACK:0x400000,0x1000", "/DYNAMICBASE:NO", "/FIXED", "/LARGEADDRESSAWARE" }

    filter {}

    links { "user32", "gdi32", "kernel32", "advapi32", "winmm" }

    filter "files:**.c"
        language "C"

    filter "configurations:Debug"
        defines { "_DEBUG" }
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines { "NDEBUG" }
        symbols "On"
        optimize "Speed"          -- /O2
        linkoptions { "/OPT:REF", "/OPT:ICF" }
