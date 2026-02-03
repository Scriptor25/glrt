#include <bvh.hxx>
#include <fstream>
#include <iostream>
#include <istream>
#include <math.hxx>
#include <obj.hxx>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

struct camera_t
{
    mat4f inv_view;
    mat4f inv_proj;
    vec3f origin;
};

int main()
{
    object_t data;

    if (std::ifstream stream("asset/cube.obj"); stream)
    {
        read_obj(stream, data, { 0.8f, 0.2f, 0.1f }, 0.8f);
        data = translation(-1.0f, 0.5f, 2.3f) * data;
    }

    if (std::ifstream stream("asset/teapot.obj"); stream)
        read_obj(stream, data, { 0.8f, 0.8f, 0.8f }, 0.1f);

    std::vector<bvh_node_t> bvh_nodes;
    std::vector<std::uint32_t> bvh_map;
    build_bvh(data, bvh_nodes, bvh_map);

    std::string vert_source;
    if (std::ifstream stream("asset/default.vert", std::istream::ate); stream)
    {
        vert_source.resize(stream.tellg());
        stream.seekg(0, std::ios::beg);
        stream.read(vert_source.data(), static_cast<std::streamsize>(vert_source.size()));
    }

    std::string frag_source;
    if (std::ifstream stream("asset/default.frag", std::istream::ate); stream)
    {
        frag_source.resize(stream.tellg());
        stream.seekg(0, std::ios::beg);
        stream.read(frag_source.data(), static_cast<std::streamsize>(frag_source.size()));
    }

    glfwInit();

    glfwDefaultWindowHints();

    const auto window = glfwCreateWindow(800, 600, "GLRT", nullptr, nullptr);

    glfwSwapInterval(0);
    glfwMakeContextCurrent(window);

    glewInit();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    GLuint sbo[5];
    glCreateBuffers(sizeof(sbo) / sizeof(GLuint), sbo);

    glNamedBufferData(
        sbo[1],
        static_cast<GLsizeiptr>(data.indices.size() * sizeof(std::uint32_t)),
        data.indices.data(),
        GL_STATIC_DRAW);
    glNamedBufferData(
        sbo[2],
        static_cast<GLsizeiptr>(data.vertices.size() * sizeof(vertex_t)),
        data.vertices.data(),
        GL_STATIC_DRAW);
    glNamedBufferData(
        sbo[3],
        static_cast<GLsizeiptr>(bvh_nodes.size() * sizeof(bvh_node_t)),
        bvh_nodes.data(),
        GL_STATIC_DRAW);
    glNamedBufferData(
        sbo[4],
        static_cast<GLsizeiptr>(bvh_map.size() * sizeof(std::uint32_t)),
        bvh_map.data(),
        GL_STATIC_DRAW);

    auto program = glCreateProgram();

    {
        auto shader = glCreateShader(GL_VERTEX_SHADER);
        auto data_ptr = vert_source.data();
        auto length = static_cast<GLint>(vert_source.size());
        glShaderSource(shader, 1, &data_ptr, &length);
        glCompileShader(shader);

        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (!status)
        {
            char buf[512];
            GLsizei len;
            glGetShaderInfoLog(shader, sizeof(buf), &len, buf);
            buf[len] = 0;
            std::cerr << buf << std::endl;
        }

        glAttachShader(program, shader);
        glDeleteShader(shader);
    }

    {
        auto shader = glCreateShader(GL_FRAGMENT_SHADER);
        auto data_ptr = frag_source.data();
        auto length = static_cast<GLint>(frag_source.size());
        glShaderSource(shader, 1, &data_ptr, &length);
        glCompileShader(shader);

        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (!status)
        {
            char buf[512];
            GLsizei len;
            glGetShaderInfoLog(shader, sizeof(buf), &len, buf);
            buf[len] = 0;
            std::cerr << buf << std::endl;
        }

        glAttachShader(program, shader);
        glDeleteShader(shader);
    }

    glLinkProgram(program);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, sbo[0]);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sbo[1]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sbo[2]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, sbo[3]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, sbo[4]);

    vec3f origin{ 4.0f, 3.0f, 6.0f };
    vec3f target{ 1.0f, 1.0f, 1.0f };

    auto view = lookAt(origin, target, { 0.0f, 1.0f, 0.0f });

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        auto proj = perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);

        const camera_t camera
        {
            .inv_view = inverse(view),
            .inv_proj = inverse(proj),
            .origin = origin,
        };

        glNamedBufferData(
            sbo[0],
            sizeof(camera_t),
            &camera,
            GL_STATIC_DRAW);

        glEnable(GL_DEPTH_TEST);

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glUseProgram(0);

        glfwSwapBuffers(window);
    }

    glDeleteProgram(program);

    glDeleteBuffers(sizeof(sbo) / sizeof(GLuint), sbo);

    glfwDestroyWindow(window);
    glfwTerminate();
}
