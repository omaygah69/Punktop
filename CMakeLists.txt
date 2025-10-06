cmake_minimum_required(VERSION 3.25)

project(
  punktop
  VERSION 1.0
  DESCRIPTION "punktop"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)

# Add your local SDL2 static library
add_library(SDL2 STATIC IMPORTED)
set_target_properties(
  SDL2
  PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/libSDL2.a
             INTERFACE_INCLUDE_DIRECTORIES
             ${CMAKE_CURRENT_SOURCE_DIR}/include/SDL2)

# ImGui library
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

target_link_libraries(imgui PUBLIC OpenGL::GL SDL2)

# ImPlot library
add_library(implot STATIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui/implot.cpp
                          ${CMAKE_CURRENT_SOURCE_DIR}/imgui/implot_items.cpp)

target_include_directories(
  implot PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/implot/
                ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/)

target_link_libraries(implot PUBLIC imgui)

# Main executable
add_executable(
  ${PROJECT_NAME}
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cpu.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/disk.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dockspace.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/memoryplot.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/net.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/proc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/systemfetch.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cpuplot.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/memoryplot.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/statsfornerds.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/system.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/api.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC imgui OpenGL::GL SDL2 implot
                                             ${JANSSON_LIBRARIES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                                  ${JANSSON_INCLUDE_DIRS})

# Dependency checks
if(NOT OpenGL_FOUND)
  message(FATAL_ERROR "OpenGL not found")
endif()
