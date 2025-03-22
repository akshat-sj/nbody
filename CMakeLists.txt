cmake_minimum_required(VERSION 3.16)
project(OpenGLProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)  # Add debug symbols for development

include(FetchContent)

# Fetch dependencies
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.4
)

FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad
    GIT_TAG v0.1.34
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG 0.9.9.8
)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.4
)

FetchContent_MakeAvailable(glfw glad glm imgui)

# Find OpenMP for parallel processing
find_package(OpenMP)

# Add ImGui backend for OpenGL and GLFW
set(IMGUI_SRC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Create ImGui library
add_library(imgui STATIC ${IMGUI_SRC})
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PRIVATE glfw glad)


set(NBODY_SRC
    src/renderer.cpp
    src/particle.cpp
    src/octree.cpp
    src/nbody_system.cpp
    src/camera.cpp
)


set(SHADER_FILES
    src/particle.vs
    src/particle.fs
)

# Add executable
add_executable(OpenGLApp ${NBODY_SRC})


target_include_directories(OpenGLApp PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${glm_SOURCE_DIR}
)


target_link_libraries(OpenGLApp PRIVATE 
    glfw 
    glad 
    glm 
    imgui
    $<$<BOOL:${OpenMP_CXX_FOUND}>:OpenMP::OpenMP_CXX>
)


foreach(SHADER ${SHADER_FILES})
    add_custom_command(
        TARGET OpenGLApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/${SHADER}
        ${CMAKE_BINARY_DIR}/${SHADER}
    )
endforeach()