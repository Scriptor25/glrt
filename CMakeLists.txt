cmake_minimum_required(VERSION 3.31)

project(glrt)

set(CMAKE_CXX_STANDARD 20)

find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

file(GLOB_RECURSE SOURCES src/*.cxx)
add_executable(glrt ${SOURCES})
target_include_directories(glrt PRIVATE include)
target_link_libraries(glrt PRIVATE glfw GLEW::GLEW OpenGL::OpenGL)
