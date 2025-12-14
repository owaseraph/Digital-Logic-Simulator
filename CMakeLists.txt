cmake_minimum_required(VERSION 3.16)
project(SDL_Engine VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
file(GLOB_RECURSE SOURCES src/*.cpp)

# OpenGL
find_package(OpenGL REQUIRED)

# --- 1. SETUP PATHS (Merged Structure) ---
# Since you merged the headers, we only need ONE include path for everything
set(SDL3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/SDL/include/SDL3")

# Define paths to the Libraries (Debug)
set(SDL3_LIBRARY_DEBUG     "${CMAKE_SOURCE_DIR}/external/SDL/libs/Debug/SDL3.lib")
set(SDL3_TTF_LIBRARY_DEBUG "${CMAKE_SOURCE_DIR}/external/SDL/libs/Debug/SDL3_ttf.lib")

# Define paths to the Libraries (Release)
# (Assuming you also copied the Release versions into external/SDL/libs/Release/)
set(SDL3_LIBRARY_RELEASE     "${CMAKE_SOURCE_DIR}/external/SDL/libs/Release/SDL3.lib")
set(SDL3_TTF_LIBRARY_RELEASE "${CMAKE_SOURCE_DIR}/external/SDL/libs/Release/SDL3_ttf.lib")

# Define paths to the DLLs (for automatic copying)
set(SDL3_DLL_DEBUG     "${CMAKE_SOURCE_DIR}/external/SDL/libs/Debug/SDL3.dll")
set(SDL3_TTF_DLL_DEBUG "${CMAKE_SOURCE_DIR}/external/SDL/libs/Debug/SDL3_ttf.dll")
# (If you don't have Release DLLs yet, you can point these to Debug or leave them, 
# but the build might warn you if you switch to Release mode)
set(SDL3_DLL_RELEASE     "${CMAKE_SOURCE_DIR}/external/SDL/libs/Release/SDL3.dll")
set(SDL3_TTF_DLL_RELEASE "${CMAKE_SOURCE_DIR}/external/SDL/libs/Release/SDL3_ttf.dll")

# Add ImGui & GLM
add_subdirectory(external/imgui)
add_subdirectory(external/glm)

# Create executable
add_executable(Digital_Sim ${SOURCES})

# Include directories
target_include_directories(Digital_Sim PRIVATE
    include
    include/components
    external/imgui
    external/glm
    ${SDL3_INCLUDE_DIR} 
)

# Link libraries
target_link_libraries(Digital_Sim PRIVATE
    imgui
    glm
    OpenGL::GL
    # Link Debug versions
    debug ${SDL3_LIBRARY_DEBUG}
    debug ${SDL3_TTF_LIBRARY_DEBUG}
    # Link Release versions
    optimized ${SDL3_LIBRARY_RELEASE}
    optimized ${SDL3_TTF_LIBRARY_RELEASE}
)

# Copy DLLs to output folder automatically
add_custom_command(TARGET Digital_Sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<$<CONFIG:Debug>:${SDL3_DLL_DEBUG}>
        $<$<CONFIG:Release>:${SDL3_DLL_RELEASE}>
        $<$<CONFIG:Debug>:${SDL3_TTF_DLL_DEBUG}>
        $<$<CONFIG:Release>:${SDL3_TTF_DLL_RELEASE}>
        $<TARGET_FILE_DIR:Digital_Sim>
)

# Copy the font.ttf file to the output folder
add_custom_command(TARGET Digital_Sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/assets/fonts/font.ttf"      
    "$<TARGET_FILE_DIR:Digital_Sim>/font.ttf"   
)