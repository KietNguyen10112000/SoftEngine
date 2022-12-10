cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

set(BINARY Test)

add_compile_definitions(UNICODE)

file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.h *.cpp)

#file(GLOB ENGINE_SOURCES ${CMAKE_SOURCE_DIR}/Engine/**/*.cpp)


add_executable(${BINARY} ${TEST_SOURCES})

include_directories(
    PUBLIC

    "${CMAKE_SOURCE_DIR}/Libraries/build/include/googletest"

    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/Engine"


    "${CMAKE_SOURCE_DIR}/Libraries/glm"
)

target_link_directories(
    ${BINARY} PUBLIC
    $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Libraries/build/bin/Debug>
    $<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/Libraries/build/bin/Release>

    $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/build/bin/Debug>
    $<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/build/bin/Release>
)

target_link_libraries(
    ${BINARY} PUBLIC

    "gtest" 
    "gtest_main" 
    "gmock" 
    "gmock_main"

    "LibEngine"
)

add_dependencies(${BINARY} Engine)


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