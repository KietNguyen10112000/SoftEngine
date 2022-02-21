# CMakeList.txt : CMake project for SoftEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

# Add source to this project's executable.

file(GLOB SOURCES1
    ${PROJECT_SOURCE_DIR}/SoftEngine/Core/*.h
    ${PROJECT_SOURCE_DIR}/SoftEngine/Core/*.cpp
    ${PROJECT_SOURCE_DIR}/SoftEngine/Core/ImportParser/*.cpp
    ${PROJECT_SOURCE_DIR}/SoftEngine/Engine/*.cpp
    ${PROJECT_SOURCE_DIR}/SoftEngine/Components/*.cpp
)

file(GLOB_RECURSE 
    LIBS_SRC
    LIST_DIRECTORIES true
    ${PROJECT_SOURCE_DIR}/SoftEngine/Libs/*/include/
)

file(GLOB_RECURSE 
    IMGUI_SRC
    LIST_DIRECTORIES true
    "${PROJECT_SOURCE_DIR}/SoftEngine/Libs/imgui/*.cpp"
)


add_executable (SoftEngine "SoftEngine.cpp" "SoftEngine.h"
	#"Source/Renderer.cpp"
    #"Source/Window.cpp"

    #"Engine/Engine.cpp"
    ${SOURCES1}
    ${IMGUI_SRC}
)

option(DX11_RENDERER "Use directX 11 as render api" ON)

if(DX11_RENDERER)
    add_compile_definitions(DX11_RENDERER)
endif()

option(WIN32_WINDOW "Use Win32 window and windows input system" ON)

if(WIN32_WINDOW)
    add_compile_definitions(WIN32_WINDOW)
endif()

add_compile_definitions(UNICODE)

target_include_directories(SoftEngine PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}/SoftEngine/Core"
                          "${PROJECT_SOURCE_DIR}/SoftEngine"
                          #"${PROJECT_SOURCE_DIR}/SoftEngine/Libs/DirectXTex/include"
                          ${LIBS_SRC}
                          "${PROJECT_SOURCE_DIR}/SoftEngine/Libs/imgui"
)

#find_library(DIRECT_X_TEX DirectXTexD "${PROJECT_SOURCE_DIR}/SoftEngine/Libs/DirectXTex/bin")

if(WIN32)
    #directXtex for windows
    if(${CMAKE_BUILD_TYPE} MATCHES Debug)
        target_link_libraries(SoftEngine PUBLIC
            "${PROJECT_SOURCE_DIR}/SoftEngine/Libs/DirectXTex/bin/DirectXTexD.lib"
        )
    else()
        target_link_libraries(SoftEngine PUBLIC
            "${PROJECT_SOURCE_DIR}/SoftEngine/Libs/DirectXTex/bin/DirectXTex.lib"
        )
    endif()
endif()


#assimp is static model importer
target_link_libraries(SoftEngine PUBLIC
    "${PROJECT_SOURCE_DIR}/SoftEngine/Libs/assimp/bin/assimp-vc142-mtd.lib"
)

#fbx_sdk for animated model
if(${CMAKE_BUILD_TYPE} MATCHES Debug)
    target_link_libraries(SoftEngine PUBLIC
        "${PROJECT_SOURCE_DIR}/SoftEngine/Libs/fbx_sdk/bin/debug/libfbxsdk.lib"
    )
else()
    target_link_libraries(SoftEngine PUBLIC
        "${PROJECT_SOURCE_DIR}/SoftEngine/Libs/fbx_sdk/bin/release/libfbxsdk.lib"
    )
endif()


# TODO: Add tests and install targets if needed.
