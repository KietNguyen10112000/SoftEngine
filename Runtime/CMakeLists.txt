cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_compile_definitions(EXPORTS UNICODE DEV)
add_compile_definitions(IMGUI_USER_CONFIG="Libraries/config/ImGUIConfig.h")

set(BINARY Engine)
#set(LIB_ENGINE LibEngine)

file(
    GLOB
    SRC


    "Core/*.natvis"
    "Core/Memory/*.h"
    "Core/Memory/*.cpp"
    "Core/Structures/*.h"
    "Core/Structures/Managed/*.h"
    "Core/Structures/Raw/*.h"
    "Core/Fiber/*.h"
    "Core/Fiber/*.cpp"
    "Core/Thread/*.h"
    "Core/Thread/*.cpp"
    "Core/Random/*.h"
    "Core/Random/*.cpp"


    "Math/*.h"
    "Math/*.natvis"


    "TaskSystem/*.h"
    "TaskSystem/*.cpp"


    "Platform/*.h"
    "Platform/*.cpp"


    "Common/Base/*.h"
    "Common/Base/*.cpp"
    "Common/ComponentQueryStructures/*.h"
    "Common/ComponentQueryStructures/*.cpp"
    "Common/QueryStructures/*.h"
    "Common/QueryStructures/*.cpp"


    "MainSystem/Rendering/*.h"
    "MainSystem/Rendering/*.cpp"
    "MainSystem/Rendering/Components/*.h"
    "MainSystem/Rendering/Components/*.cpp"
    "MainSystem/Rendering/RenderingPipeline/*.h"
    "MainSystem/Rendering/RenderingPipeline/*.cpp"
    "MainSystem/Physics/*.h"
    "MainSystem/Physics/*.cpp"
    "MainSystem/Scripting/*.h"
    "MainSystem/Scripting/*.cpp"
    "MainSystem/Audio/*.h"
    "MainSystem/Audio/*.cpp"
    "MainSystem/Animation/*.h"
    "MainSystem/Animation/*.cpp"


    "Scene/*.h"
    "Scene/*.cpp"
	

    "Modules/Graphics/*.h"
    "Modules/Graphics/*.cpp"
    "Modules/Graphics/Detail/Detail.cpp"

    "Modules/Network/*.h"
    "Modules/Network/*.cpp"
    "Modules/Network/API/API.h"
    "Modules/Network/API/API.cpp"

    "Modules/FileSystem/*.h"
    "Modules/FileSystem/*.cpp"

    "Modules/Plugins/*.h"
    "Modules/Plugins/*.cpp"
    "Modules/Plugins/Bridge/*.cpp"

    "Modules/Resources/*.h"
    "Modules/Resources/*.cpp"

    "Runtime.h"
    "Runtime.cpp"
    "StartupConfig.h"
    "StartupConfig.cpp"
)



IF (WIN32)
    # for imgui
    file(
        GLOB
        IMGUI_SRC

        "../Libraries/imgui/*.cpp"
        "../Libraries/imgui/backends/imgui_impl_win32.cpp"
        "../Libraries/imgui/backends/imgui_impl_dx12.cpp"
    )
ELSE()
    message( FATAL_ERROR "unsupported, yet!" )
ENDIF()

 add_executable (
    ${BINARY} 
    "main.cpp"
    ${SRC}

    ${IMGUI_SRC}
)

include_directories(PUBLIC
     "${CMAKE_CURRENT_SOURCE_DIR}"
     "${CMAKE_CURRENT_SOURCE_DIR}/Modules"
     "${CMAKE_SOURCE_DIR}"

     "${CMAKE_SOURCE_DIR}/Libraries"
     "${CMAKE_SOURCE_DIR}/Libraries/glm"
     "${CMAKE_SOURCE_DIR}/Libraries/imgui"
     "${CMAKE_SOURCE_DIR}/Libraries/D3D12MemoryAllocator/include"
     "${CMAKE_SOURCE_DIR}/Libraries/stb"
)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(GT "")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(GT "")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set(GT "")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(GT "/GT")
endif()

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GT}")
#SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}")