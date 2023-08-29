cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_compile_definitions(EXPORTS UNICODE DEV IMGUI_SFML_SHARED_LIB)
add_compile_definitions(IMGUI_USER_CONFIG="Libraries/config/ImGUIConfig.h")

set(BINARY Engine)
set(LIB_ENGINE LibEngine)

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

    "Engine.h"
    "Engine.cpp"
    "StartupConfig.h"
    "StartupConfig.cpp"
)



IF (WIN32)
    # for imgui
    file(
        GLOB
        IMGUI_SRC
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

find_package(OpenGL REQUIRED)
include_directories( ${OPENGL_INCLUDE_DIRS})


include_directories(PUBLIC
     "${CMAKE_CURRENT_SOURCE_DIR}"
     "${CMAKE_CURRENT_SOURCE_DIR}/Modules"
     "${CMAKE_SOURCE_DIR}"

     "${CMAKE_SOURCE_DIR}/Libraries"
     "${CMAKE_SOURCE_DIR}/Libraries/glm"
     "${CMAKE_SOURCE_DIR}/Libraries/imgui"
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