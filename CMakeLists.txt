cmake_minimum_required(VERSION 3.0)

project(polydecomp VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

find_package(glfw3 3.3 REQUIRED)
find_package(glm)

include_directories(glad/include)
include_directories(learnopengl)
include_directories(earcut)

add_executable(polydecomp main.cpp glad/src/glad.c point.cpp common.cpp)
target_link_libraries(polydecomp glfw glm)

