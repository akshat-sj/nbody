cmake_minimum_required(VERSION 3.16)
project(OpenGLProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

FetchContent_MakeAvailable(glfw glad glm)

add_executable(OpenGLApp src/renderer.cpp)
target_link_libraries(OpenGLApp PRIVATE glfw glad glm)
