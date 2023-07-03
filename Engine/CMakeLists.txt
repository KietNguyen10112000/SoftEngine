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

    "Objects2D/*.h"
    "Objects2D/*.cpp"
    "Objects2D/**/*.h"
    "Objects2D/**/*.cpp"
    "Objects2D/**/**/*.h"
    "Objects2D/**/**/*.cpp"

    "Components2D/**/*.h"
    "Components2D/**/*.cpp"
    "Components2D/*.h"
    "Components2D/*.cpp"

    "SubSystems2D/**/*.h"
    "SubSystems2D/**/*.cpp"
    "SubSystems2D/*.h"
    "SubSystems2D/*.cpp"


    "TaskSystem/*.h"
    "TaskSystem/*.cpp"


    "Platform/*.h"
    "Platform/*.cpp"


    "Modules/Graphics2D/*.h"
    "Modules/Graphics2D/*.cpp"

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

        "../Libraries/imgui/*.cpp"
        "../Libraries/imgui-SFML/*.cpp"
        #"../Libraries/imgui/backends/imgui_impl_win32.cpp"
        #"../Libraries/imgui/backends/imgui_impl_dx12.cpp"
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

     "${CMAKE_SOURCE_DIR}/Libraries/SFML/SFML-2.5.1/include"
     "${CMAKE_SOURCE_DIR}/Libraries/imgui-SFML"
)

target_link_directories(
    ${BINARY} PUBLIC 
    "${CMAKE_SOURCE_DIR}/Libraries/SFML/SFML-2.5.1/lib"
)

set(
    SFML_LIBS_RELEASE
    "flac.lib"
    "freetype.lib"
    "ogg.lib"
    "openal32.lib"
    "sfml-audio.lib"
    #"sfml-audio-d.lib"
    #"sfml-audio-s.lib"
    #"sfml-audio-s-d.lib"
    "sfml-graphics.lib"
    #"sfml-graphics-d.lib"
    #"sfml-graphics-s.lib"
    #"sfml-graphics-s-d.lib"
    "sfml-main.lib"
    #"sfml-main-d.lib"
    "sfml-network.lib"
    #"sfml-network-d.lib"
    #"sfml-network-s.lib"
    #"sfml-network-s-d.lib"
    "sfml-system.lib"
    #"sfml-system-d.lib"
    #"sfml-system-s.lib"
    #"sfml-system-s-d.lib"
    "sfml-window.lib"
    #"sfml-window-d.lib"
    #"sfml-window-s.lib"
    #"sfml-window-s-d.lib"
    #"vorbis.lib"
    #"vorbisenc.lib"
    #"vorbisfile.lib"
)

set(
    SFML_LIBS_DEBUG
    "flac.lib"
    "freetype.lib"
    "ogg.lib"
    "openal32.lib"
    #"sfml-audio.lib"
    "sfml-audio-d.lib"
    #"sfml-audio-s.lib"
    #"sfml-audio-s-d.lib"
    #"sfml-graphics.lib"
    "sfml-graphics-d.lib"
    #"sfml-graphics-s.lib"
    #"sfml-graphics-s-d.lib"
    #"sfml-main.lib"
    "sfml-main-d.lib"
    #"sfml-network.lib"
    "sfml-network-d.lib"
    #"sfml-network-s.lib"
    #"sfml-network-s-d.lib"
    #"sfml-system.lib"
    "sfml-system-d.lib"
    #"sfml-system-s.lib"
    #"sfml-system-s-d.lib"
    #"sfml-window.lib"
    "sfml-window-d.lib"
    #"sfml-window-s.lib"
    #"sfml-window-s-d.lib"
    #"vorbis.lib"
    #"vorbisenc.lib"
    #"vorbisfile.lib"
)

target_link_libraries(
    ${BINARY} 
    

    $<$<CONFIG:Debug>:${SFML_LIBS_DEBUG}>
    $<$<CONFIG:Release>:${SFML_LIBS_RELEASE}>

    ${OPENGL_LIBRARIES}
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