cmake_minimum_required( VERSION 3.21 )

project( cod4rad LANGUAGES C CXX )

if( NOT MSVC )
    message( FATAL_ERROR
        "cod4rad requires MSVC.  The original is an x87 MSVC build and other "
        "compilers change floating-point results." )
endif()

if( NOT CMAKE_SIZEOF_VOID_P EQUAL 4 )
    message( FATAL_ERROR
        "cod4rad must be built for 32-bit x86.  Configure with:  "
        "cmake -B build -A Win32" )
endif()

if( NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE )
endif()

set( CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY         "${CMAKE_SOURCE_DIR}/bin" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/bin" )

set( SRC_COD4RAD
    cod4rad/cmdline.cpp
    cod4rad/cod4rad.cpp
    cod4rad/compile.cpp
    cod4rad/geometry.cpp
    cod4rad/groundlight.cpp
    cod4rad/lightgrid.cpp
    cod4rad/lighting.cpp
    cod4rad/lightmap_bleed.cpp
    cod4rad/mapio.cpp
    cod4rad/maskedmaterial.cpp
    cod4rad/modelcollision.cpp
    cod4rad/pointlights.cpp
    cod4rad/progress.cpp
)

set( SRC_SCRIPT
    src/script/scr_memorytree.cpp
    src/script/scr_stringlist.cpp
)

set( SRC_GFX
    src/gfx_d3d/r_imagedecode.cpp
    src/gfx_d3d/r_light_load_obj.cpp
    src/gfx_d3d/r_image_wavelet.cpp
    src/gfx_d3d/r_image_wavelet_tables.cpp
    src/gfx_d3d/r_xsurface.cpp
    src/gfx_d3d/r_xsurface_load_obj.cpp
)

set( SRC_XANIM
    src/xanim/dobj.cpp
    src/xanim/xmodel.cpp
    src/xanim/xmodel_load_obj.cpp
    src/xanim/xmodel_utils.cpp
)

set( SRC_QCOMMON
    src/qcommon/cm_tracebox.cpp
)

set( SRC_COMMON
    common/bspfile.cpp
    common/brush_edges.cpp
    common/collvec.cpp
    common/poly2d.cpp
    common/polylib.cpp
    common/threads.cpp
    common/targetplatform.cpp
)

set( SRC_COD4MAP
    cod4map/materials.cpp
)

set( SRC_CMDLIB
    libs/cmdlib/cmdlib.cpp
)

set( SRC_UNIVERSAL
    src/universal/aabbtree.cpp
    src/universal/assertive.cpp
    src/universal/com_convexhull.cpp
    src/universal/com_crc32.cpp
    src/universal/com_files.cpp
    src/universal/com_math.cpp
    src/universal/com_memory.cpp
    src/universal/com_pack.cpp
    src/universal/com_shared.cpp
    src/universal/linearmapping.cpp
    src/universal/com_vector.cpp
    src/universal/dvar.cpp
    src/universal/q_parse.cpp
    src/universal/q_shared.cpp
    src/universal/surfaceflags.cpp
)

set( SRC_ODE
    src/physics/ode/src/mass.cpp
    src/physics/ode/src/matrix.cpp
)

set( SRC_ZLIB
    src/zlib/adler32.c
    src/zlib/compress.c
    src/zlib/crc32.c
    src/zlib/deflate.c
    src/zlib/inffast.c
    src/zlib/inflate.c
    src/zlib/inftrees.c
    src/zlib/trees.c
    src/zlib/uncompr.c
    src/zlib/unzip.c
    src/zlib/zutil.c
)

add_executable( cod4rad
    ${SRC_COD4RAD}
    ${SRC_COMMON}
    ${SRC_COD4MAP}
    ${SRC_CMDLIB}
    ${SRC_UNIVERSAL}
    ${SRC_ODE}
    ${SRC_SCRIPT}
    ${SRC_QCOMMON}
    ${SRC_GFX}
    ${SRC_XANIM}
    ${SRC_ZLIB}
)

target_include_directories( cod4rad PRIVATE
    cod4rad
    cod4map
    common
    libs/cmdlib
    src
    src/universal
    src/qcommon
    src/gfx_d3d
    src/xanim
    src/script
    src/zlib
)

target_compile_definitions( cod4rad PRIVATE
    WIN32
    _WINDOWS
    _CRT_SECURE_NO_WARNINGS
    $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
)

target_compile_options( cod4rad PRIVATE
    /arch:IA32
    /fp:precise
    /GS-
    /Gy
    /W3
    /wd4996 /wd4244 /wd4018 /wd4267 /wd4305 /wd4101 /wd4700
    $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
    $<$<NOT:$<CONFIG:Debug>>:/O2>
)

target_link_options( cod4rad PRIVATE
    /SUBSYSTEM:CONSOLE
    /STACK:0x400000,0x1000
    /DYNAMICBASE:NO
    /FIXED
    /LARGEADDRESSAWARE
    /OPT:REF
    /OPT:ICF
)

target_link_libraries( cod4rad PRIVATE
    user32 gdi32 kernel32 advapi32 winmm
)

source_group( "cod4rad"        FILES ${SRC_COD4RAD} )
source_group( "common"         FILES ${SRC_COMMON} )
source_group( "cod4map"        FILES ${SRC_COD4MAP} )
source_group( "libs/cmdlib"    FILES ${SRC_CMDLIB} )
source_group( "src/universal"  FILES ${SRC_UNIVERSAL} )
source_group( "src/physics"    FILES ${SRC_ODE} )
source_group( "src/script"     FILES ${SRC_SCRIPT} )
source_group( "src/qcommon"    FILES ${SRC_QCOMMON} )
source_group( "src/gfx_d3d"    FILES ${SRC_GFX} )
source_group( "src/xanim"      FILES ${SRC_XANIM} )
source_group( "src/zlib"       FILES ${SRC_ZLIB} )
