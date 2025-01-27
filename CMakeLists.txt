cmake_minimum_required(VERSION 3.24)
project(draft)

include(FetchContent)
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0.9.9.8
)
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.89.2
)
FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
)
FetchContent_MakeAvailable(glm imgui stb)

set(DRAFT_LIBS glfw GLEW glm GL)
set(CMAKE_CXX_STANDARD 23)

add_subdirectory(src)
