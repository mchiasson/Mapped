cmake_minimum_required(VERSION 3.0.0)

# Define LINUX
if (UNIX AND NOT APPLE)
    set(LINUX 1)
endif()

# Setup modules path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

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

#justwindowsthings
if (WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Project files
file(GLOB srcfiles
    ./src/*.*
)
file(GLOB systemsFiles
    ./src/systems/*.*
)

list(APPEND includes PUBLIC ./src/ ./src/systems/)

# Dear imgui
list(APPEND includes PUBLIC ./thirdparty/imgui/ ./thirdparty/imgui/examples/)
list(APPEND srcthirdparty
    ./src/imconfig.h
    ./thirdparty/imgui/imgui_demo.cpp
    ./thirdparty/imgui/imgui_draw.cpp
    ./thirdparty/imgui/imgui_internal.h
    ./thirdparty/imgui/imgui.cpp
    ./thirdparty/imgui/imgui.h
    ./thirdparty/imgui/stb_rect_pack.h
    ./thirdparty/imgui/stb_textedit.h
    ./thirdparty/imgui/stb_truetype.h
    ./thirdparty/imgui/examples/imgui_impl_opengl3.cpp
    ./thirdparty/imgui/examples/imgui_impl_opengl3.h
    ./thirdparty/imgui/examples/imgui_impl_sdl.cpp
    ./thirdparty/imgui/examples/imgui_impl_sdl.h
)

# SDL
set(HAVE_LIBC ON)
add_subdirectory(./thirdparty/SDL/)
list(APPEND libs SDL2-static)
list(APPEND includes PUBLIC ./thirdparty/SDL/include/)

# OpenGL
find_package(OpenGL REQUIRED)
list(APPEND includes PUBLIC ${OPENGL_INCLUDE_DIR})
list(APPEND libs ${OPENGL_LIBRARIES})

# assimp
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(./thirdparty/assimp/)
list(APPEND libs assimp)

# gl3w
list(APPEND includes PUBLIC ./thirdparty/gl3w/include/)
list(APPEND srcthirdparty ./thirdparty/gl3w/src/gl3w.c)

# jsoncpp
list(APPEND includes PUBLIC ./thirdparty/jsoncpp/include/)
list(APPEND srcthirdparty
    ./thirdparty/jsoncpp/src/lib_json/json_reader.cpp
    ./thirdparty/jsoncpp/src/lib_json/json_tool.h
    ./thirdparty/jsoncpp/src/lib_json/json_value.cpp
    ./thirdparty/jsoncpp/src/lib_json/json_valueiterator.inl
    ./thirdparty/jsoncpp/src/lib_json/json_writer.cpp
)

# tinyfiledialogs
list(APPEND includes PUBLIC ./thirdparty/tinyfiledialogs/)
list(APPEND srcthirdparty ./thirdparty/tinyfiledialogs/tinyfiledialogs.c)

# STB
list(APPEND includes PUBLIC ./thirdparty/stb/)

# MapEditor.exe, use WinMain on Windows
source_group("thirdparty" FILES ${srcthirdparty})
source_group("editor" FILES ${srcfiles})
source_group("systems" FILES ${systemsFiles})
add_executable(MapEditor WIN32 ${srcfiles} ${systemsFiles} ${srcthirdparty})

# Lib/Headers
target_include_directories(MapEditor ${includes})
target_link_libraries(MapEditor ${libs})
