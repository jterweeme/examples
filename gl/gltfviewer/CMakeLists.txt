cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0048 NEW)

project(gltf-viewer VERSION 0.0.1)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(
        -std=c++14
    )
endif()

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

message(STATUS "Preparing solution with CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#set(IMGUI_DIR imgui-1.74)
set(GLAD_DIR glad)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(c2ba-glsl-shaders)
include(c2ba-copy-dll-post-build)
include(c2ba-assets-directory)

set(SHADER_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/shaders)
set(ASSET_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/assets)

set(
    THIRD_PARTY_SRC_FILES
    #${IMGUI_SRC_FILES}
    third-party/${GLAD_DIR}/src/glad.c
)

set(
    LIBRARIES
    ${OPENGL_LIBRARIES}
    glfw
    imgui
    stb
)

set(CXXFLAGS ${CXXFLAGS} std=c++14)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CXXFLAGS ${CXXFLAGS} std=c++17)
    set(LIBRARIES ${LIBRARIES} stdc++fs)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "8.0.0")
        set(USE_STD_FILESYSTEM 1)
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES "[Cc]lang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0.0")
        set(LIBRARIES ${LIBRARIES} stdc++fs)
    else()
        set(CXXFLAGS ${CXXFLAGS} std=c++17)
        set(USE_STD_FILESYSTEM 1)
    endif()
endif()

source_group("glsl" REGULAR_EXPRESSION ".*/*.glsl")
source_group("third-party" REGULAR_EXPRESSION "third-party/*.*")

set(APP gltf-viewer)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

file(
    GLOB_RECURSE
    SRC_FILES
    ${SRC_DIR}/*
)

add_executable(
    ${APP}
    ${SRC_FILES}
    ${THIRD_PARTY_SRC_FILES}
)

target_include_directories(
    ${APP}
    PUBLIC
    ${OPENGL_INCLUDE_DIRS}
    third-party/${GLAD_DIR}/include
    lib/include
    /usr/include/stb
    /usr/include/imgui
    /usr/include/imgui/backends
)

target_compile_definitions(
    ${APP}
    PUBLIC
    IMGUI_IMPL_OPENGL_LOADER_GLAD
    GLM_ENABLE_EXPERIMENTAL
)

target_link_libraries(
    ${APP}
    ${LIBRARIES}
)

install(
    TARGETS ${APP}
    DESTINATION .
)

c2ba_add_shader_directory(${SRC_DIR}/shaders ${SHADER_OUTPUT_PATH})
c2ba_add_assets_directory(${SRC_DIR}/assets ${ASSET_OUTPUT_PATH})

install(
    DIRECTORY ${SRC_DIR}/shaders/
    DESTINATION shaders/
    FILES_MATCHING PATTERN "*.glsl"
)

