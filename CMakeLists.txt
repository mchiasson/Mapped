cmake_minimum_required(VERSION 3.0.0)

# Define LINUX
if (UNIX AND NOT APPLE)
    set(LINUX 1)
endif()

# Setup modules path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# Setup Hunter cmake package manager
include(HunterGate)
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.23.24.tar.gz"
    SHA1 "75f3b4507be7375370b25fa183c9c955e80ccf9c"
    LOCAL # tells hunter to load cmake/Hunter/config.cmake
)

# Project name
project(MapEditor)

# Some compiler flags
set(CMAKE_CXX_STANDARD 11) # C++11
if (MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" "/MP") # Multi core in VS
endif()

# Define _DEBUG
if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-D_DEBUG)
endif()

# Project files
file(GLOB srcfiles
    ./src/*.*
)
file(GLOB systemsFiles
    ./src/systems/*.*
)

list(APPEND includes PUBLIC ./src/ ./src/systems/)

# Dear imgui (https://docs.hunter.sh/en/latest/packages/pkg/imgui.html)
hunter_add_package(imgui)
find_package(imgui CONFIG REQUIRED)
list(APPEND libs imgui::imgui)

# SDL2 (https://docs.hunter.sh/en/latest/packages/pkg/SDL2.html)
hunter_add_package(SDL2)
find_package(SDL2 CONFIG REQUIRED)
list(APPEND libs SDL2::SDL2 SDL2::SDL2main)

# OpenGL
find_package(OpenGL REQUIRED)
list(APPEND libs OpenGL::GL)

# assimp
option(BUILD_SHARED_LIBS "Disabling Shared libraries." OFF)
add_subdirectory(./thirdparty/assimp/)
list(APPEND libs assimp)

# gl3w
list(APPEND includes PUBLIC ./thirdparty/gl3w/include/)
list(APPEND srcthirdparty ./thirdparty/gl3w/src/gl3w.c)

# jsoncpp (https://docs.hunter.sh/en/latest/packages/pkg/jsoncpp.html)
hunter_add_package(jsoncpp)
find_package(jsoncpp CONFIG REQUIRED)
list(APPEND libs jsoncpp_lib_static)

# tinyfiledialogs
list(APPEND includes PUBLIC ./thirdparty/tinyfiledialogs/)
list(APPEND srcthirdparty ./thirdparty/tinyfiledialogs/tinyfiledialogs.c)

# stb (https://docs.hunter.sh/en/latest/packages/pkg/stb.html)
hunter_add_package(stb)
find_package(stb CONFIG REQUIRED)
list(APPEND libs stb::stb)

# MapEditor.exe, use WinMain on Windows
source_group("thirdparty" FILES ${srcthirdparty})
source_group("editor" FILES ${srcfiles})
source_group("systems" FILES ${systemsFiles})
add_executable(MapEditor WIN32 ${srcfiles} ${systemsFiles} ${srcthirdparty})

# Lib/Headers
target_include_directories(MapEditor ${includes})
target_link_libraries(MapEditor ${libs})

#justwindowsthings
if (WIN32)
    target_compile_definitions(MapEditor PUBLIC -DNOMINMAX)
    target_compile_definitions(MapEditor PUBLIC -D_CRT_SECURE_NO_WARNINGS)
endif()
