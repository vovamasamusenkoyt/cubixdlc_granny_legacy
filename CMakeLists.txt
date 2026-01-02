cmake_minimum_required(VERSION 3.16)
project(dx11_hook VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Build as DLL
add_library(dx11_hook SHARED
    dllmain.cpp
    
    # Hook render module
    hook_render/hook_render.cpp
    
    # Modules
    modules/watermark/watermark.cpp
    modules/esp/esp.cpp
    modules/noclip/noclip.cpp
    modules/invisible/invisible.cpp
    modules/escapes/escapes.cpp
    modules/itemspawner/itemspawner.cpp
    
    # HUD module
    hud/hud.cpp
    
    # Config module
    config/config.cpp
    
    # Utils
    utils/logger.cpp
    
    # Resources
    resources.rc
    
    # ImGui sources
    deps/imgui/imgui.cpp
    deps/imgui/imgui_demo.cpp
    deps/imgui/imgui_draw.cpp
    deps/imgui/imgui_tables.cpp
    deps/imgui/imgui_widgets.cpp
    
    # ImGui backends
    deps/imgui/backends/imgui_impl_dx11.cpp
    deps/imgui/backends/imgui_impl_win32.cpp
    
    # MinHook sources
    deps/minhook/src/buffer.c
    deps/minhook/src/hook.c
    deps/minhook/src/trampoline.c
)

# MinHook HDE source (64-bit only for x64 target)
target_sources(dx11_hook PRIVATE deps/minhook/src/hde/hde64.c)

# Include directories
target_include_directories(dx11_hook PRIVATE
    deps/imgui
    deps/imgui/backends
    deps/minhook/include
    deps/minhook/src
    deps/minhook/src/hde
    deps/IL2CPP_Resolver
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(dx11_hook PRIVATE
        _WINDLL
        UNICODE
        _UNICODE
    )
    
    # Link libraries
    target_link_libraries(dx11_hook PRIVATE
        d3d11
        dxgi
        gdi32
        user32
        winmm
        shell32
    )
    
    # Set DLL output name
    set_target_properties(dx11_hook PROPERTIES
        OUTPUT_NAME "cubixdlc"
        PREFIX ""
    )
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(dx11_hook PRIVATE
        /W4
        /WX-
        /MP
        /permissive-
    )
    
    # Link MinHook statically
    target_compile_definitions(dx11_hook PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
    
    # Suppress duplicate symbol errors from IL2CPP_Resolver (header-only library issue)
    # /FORCE:MULTIPLE - force link with duplicates
    # /IGNORE:4006 - ignore duplicate symbol warning
    # /IGNORE:4088 - ignore "image may not run" warning
    set_target_properties(dx11_hook PROPERTIES
        LINK_FLAGS "/FORCE:MULTIPLE /IGNORE:4006 /IGNORE:4088"
    )
endif()

# GCC/Clang options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dx11_hook PRIVATE
        -Wall
        -Wextra
        -Wno-unused-parameter
    )
endif()

