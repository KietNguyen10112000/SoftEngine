cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_compile_definitions(EXPORTS UNICODE DEV)

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

    "Objects/*.h"
    "Objects/*.cpp"
    "Objects/**/*.h"
    "Objects/**/*.cpp"
    "Objects/**/**/*.h"
    "Objects/**/**/*.cpp"

    "Components/**/*.h"
    "Components/**/*.cpp"
    "Components/*.h"
    "Components/*.cpp"

    "SubSystems/**/*.h"
    "SubSystems/**/*.cpp"
    "SubSystems/*.h"
    "SubSystems/*.cpp"


    "TaskSystem/*.h"
    "TaskSystem/*.cpp"


    "Platform/*.h"
    "Platform/*.cpp"


    "Modules/Graphics/*.h"
    "Modules/Graphics/*.cpp"
    "Modules/Graphics/API/API.h"
    "Modules/Graphics/API/API.cpp"

    "Modules/Network/*.h"
    "Modules/Network/*.cpp"
    "Modules/Network/API/API.h"
    "Modules/Network/API/API.cpp"

    "Modules/FileSystem/*.h"
    "Modules/FileSystem/*.cpp"

    "Modules/Plugins/*.h"
    "Modules/Plugins/*.cpp"
    "Modules/Plugins/Bridge/*.cpp"

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

add_library (
    ${LIB_ENGINE} SHARED 
    "main.cpp"
    ${SRC}
)

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

#SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GT}")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}")