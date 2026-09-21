cmake_minimum_required(VERSION 3.20)
project(MinecraftCloneGame LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(dependencies/FastNoise2)
# Tell the Ninja compiler where your manual folders live
include_directories(include)
link_directories(lib)

add_executable(${PROJECT_NAME} code/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
    FastNoise
    glfw3
    opengl32
    gdi32
    winmm
)

target_include_directories(${PROJECT_NAME} PRIVATE
    glm
)

target_sources(${PROJECT_NAME} PRIVATE
    src/glad.c
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    SHADER_DIR="${CMAKE_SOURCE_DIR}/src/Shaders/"
    TEXTURE_DIR="${CMAKE_SOURCE_DIR}/src/textures/"
)
