cmake_minimum_required(VERSION 3.25)

project(
  Punktop
  VERSION 1.0
  DESCRIPTION "punktop"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED CONFIG)

add_library(
  imgui STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_opengl3.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_sdl2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_sdlrenderer2.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp)

target_include_directories(imgui
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui)

target_link_libraries(imgui PUBLIC glfw OpenGL::GL SDL2::SDL2)

add_library(implot STATIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui/implot.cpp
                          ${CMAKE_CURRENT_SOURCE_DIR}/imgui/implot_items.cpp)

target_include_directories(
  implot PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/implot/
                ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/)

target_link_libraries(implot PUBLIC imgui)

add_executable(
  ${PROJECT_NAME}
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cpu.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/disk.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dockspace.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/memoryplot.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/proc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/systemfetch.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC imgui glfw OpenGL::GL SDL2::SDL2
                                             implot)

if(NOT glfw3_FOUND)
  message(FATAL_ERROR "GLFW3 not found.")
endif()
if(NOT OpenGL_FOUND)
  message(FATAL_ERROR "OpenGL not found")
endif()
if(NOT SDL2_FOUND)
  message(FATAL_ERROR "SDL2 not found")
endif()
