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

function(compile_shader SHADER_SOURCE SHADER_BINARY)
    add_custom_command(
            OUTPUT ${SHADER_BINARY}
            DEPENDS ${SHADER_SOURCE}
            COMMAND glslc --target-env=opengl -std=450core -o ${SHADER_BINARY} ${SHADER_SOURCE}
            COMMENT "Building SPV ${SHADER_BINARY}"
            VERBATIM
    )
endfunction()

set(SHADER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/asset/shader/default.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/asset/shader/default.frag
        ${CMAKE_CURRENT_SOURCE_DIR}/asset/shader/default.comp
)
set(SHADER_BINARIES)

foreach (SHADER_SOURCE IN LISTS SHADER_SOURCES)
    compile_shader(${SHADER_SOURCE} ${SHADER_SOURCE}.spv)
    list(APPEND SHADER_BINARIES ${SHADER_SOURCE}.spv)
endforeach ()

add_custom_target(shader_binaries DEPENDS ${SHADER_BINARIES})

add_dependencies(glrt shader_binaries)
