cmake_minimum_required(VERSION 3.16)

project(imgui)

file(GLOB IMGUI_SRC
    *.cpp
    backends/imgui_impl_sdl3.cpp
    backends/imgui_impl_opengl3.cpp
    backends/imgui_impl_sdlrenderer3.cpp
    )


add_library(imgui ${IMGUI_SRC})


target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
    ${CMAKE_SOURCE_DIR}/external/SDL/include
    )

target_link_libraries(imgui PUBLIC
    OpenGL::GL
)