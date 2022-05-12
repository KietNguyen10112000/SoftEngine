# CMakeList.txt : CMake project for SoftEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

# Add source to this project's executable.

file(
    GLOB

    SOFT_ENGINE_SRC

    "${PROJECT_SOURCE_DIR}/SoftEngine/Core/*.h"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Core/*.cpp"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Core/ImportParser/*.cpp"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Core/MultiThreading/*.cpp"

    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/*.cpp"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Components/*.cpp"

    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/Rendering/*.cpp"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/Rendering/*.h"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/Logic/*.cpp"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/Logic/*.h"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/Scene/*.cpp"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/Scene/*.h"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/Controllers/*.cpp"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/Controllers/*.h"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/UI/*.cpp"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/UI/*.h"

    # Bullet 3 wrapper
    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/Physics/Modules/Bullet/*.cpp"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/Physics/Modules/Bullet/*.h"

    #JavaScript lang
    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/Scripting/Modules/JavaScript/*.cpp"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/Scripting/Modules/JavaScript/*.h"
    # v8 engine wrapper
    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/Scripting/Modules/JavaScript/v8/*.cpp"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Engine/Scripting/Modules/JavaScript/v8/*.h"
)

set(
    LIBS_INCLUDES

    # Assimp
    "${PROJECT_SOURCE_DIR}/SoftEngine/Libs/assimp/include/"

    # DirectXTex
    "${PROJECT_SOURCE_DIR}/SoftEngine/Libs/DirectXTex/include/"

    # ImGUI
    "${PROJECT_SOURCE_DIR}/SoftEngine/Libs/imgui/"

    # v8
    "${PROJECT_SOURCE_DIR}/SoftEngine/Libs/v8/v8-lib/include/"

    # Bullet 3
    "${PROJECT_SOURCE_DIR}/SoftEngine/Libs/bullet3/include/"
)

file(
    GLOB_RECURSE 
    IMGUI_SRC
    LIST_DIRECTORIES true

    "${PROJECT_SOURCE_DIR}/SoftEngine/Libs/imgui/*.cpp"
)


add_executable (
    SoftEngine 
    "SoftEngine.cpp" "SoftEngine.h"
    ${SOFT_ENGINE_SRC}
    ${IMGUI_SRC}
)

#if(WIN32)
#    if(${CMAKE_BUILD_TYPE} MATCHES Debug)
#        set_property(TARGET SoftEngine PROPERTY
#            MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
#    else()
#        set_property(TARGET SoftEngine PROPERTY
#            MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
#    endif()
#endif()


# if this option is off, use self-written refcounted class
option(USE_STL_SMART_POINTERS "Use std::shared_ptr, std::weak_ptr, ..." ON)

if(USE_STL_SMART_POINTERS)
    add_compile_definitions(USE_STL_SMART_POINTERS)
endif()


option(DX11_RENDERER "Use directX 11 as render api" ON)

if(DX11_RENDERER)
    add_compile_definitions(DX11_RENDERER)
endif()


option(WIN32_WINDOW "Use Win32 window and windows input system" ON)

if(WIN32_WINDOW)
    add_compile_definitions(WIN32_WINDOW)
endif()


option(LOCAL_RELEASE "Local relase build" ON)
if(LOCAL_RELEASE)
    add_compile_definitions(LOCAL_RELEASE)
endif()


add_compile_definitions(NOMINMAX)
add_compile_definitions(UNICODE)
add_compile_definitions(IMGUI)


target_include_directories(
    SoftEngine PUBLIC
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/SoftEngine/Core"
    "${PROJECT_SOURCE_DIR}/SoftEngine"
    ${LIBS_INCLUDES}
)

#find_library(DIRECT_X_TEX DirectXTexD "${PROJECT_SOURCE_DIR}/SoftEngine/Libs/DirectXTex/bin")

if(WIN32)
    # DirectXtex for windows
    target_link_libraries(SoftEngine PUBLIC
        $<$<CONFIG:Debug>:${PROJECT_SOURCE_DIR}/SoftEngine/Libs/DirectXTex/bin/DirectXTexD.lib>
        $<$<CONFIG:Release>:${PROJECT_SOURCE_DIR}/SoftEngine/Libs/DirectXTex/bin/DirectXTex.lib>
    )

    # Assimp is model importer
    target_link_libraries(SoftEngine PUBLIC
         $<$<CONFIG:Debug>:${PROJECT_SOURCE_DIR}/SoftEngine/Libs/assimp/bin/assimp-vc142-mtd.lib>
         $<$<CONFIG:Release>:${PROJECT_SOURCE_DIR}/SoftEngine/Libs/assimp/bin/assimp-vc142-mtd.lib>
    )

    # v8 engine
    target_link_directories(SoftEngine PUBLIC
        $<$<CONFIG:Debug>:${PROJECT_SOURCE_DIR}/SoftEngine/Libs/v8/v8-lib/bin/debug/>
        $<$<CONFIG:Release>:${PROJECT_SOURCE_DIR}/SoftEngine/Libs/v8/v8-lib/bin/release/>
    )

    # Bullet 3
    target_link_directories(SoftEngine PUBLIC
        $<$<CONFIG:Debug>:${PROJECT_SOURCE_DIR}/SoftEngine/Libs/bullet3/bin/MD/Debug/>
        $<$<CONFIG:Release>:${PROJECT_SOURCE_DIR}/SoftEngine/Libs/bullet3/bin/MD/Release/>
    )
endif()


# TODO: Add tests and install targets if needed.
