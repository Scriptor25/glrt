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
    std::uint32_t frame{};
};

struct context_t
{
    camera_t camera;
    GLuint vertex_array{};
    GLuint buffers[5]{};
    GLuint accumulation{};
};

static void framebuffer_size_callback(GLFWwindow *window, const int width, const int height)
{
    const auto context = static_cast<context_t *>(glfwGetWindowUserPointer(window));

    context->camera.frame = {};

    const auto proj = perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
    context->camera.inv_proj = inverse(proj);

    glDeleteTextures(1, &context->accumulation);
    glCreateTextures(GL_TEXTURE_2D, 1, &context->accumulation);
    glTextureStorage2D(context->accumulation, 1, GL_RGBA32F, width, height);
    glClearTexImage(context->accumulation, 0, GL_RGBA, GL_FLOAT, nullptr);

    glBindImageTexture(
        0,
        context->accumulation,
        0,
        false,
        0,
        GL_READ_WRITE,
        GL_RGBA32F);

    glViewport(0, 0, width, height);
}

static void debug_callback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar *message,
    const void *userParam)
{
    auto context = static_cast<const context_t *>(userParam);

    std::cerr << message << std::endl;
}

int main()
{
    constexpr vec3f origin{ 4.0f, 3.0f, 6.0f };
    constexpr vec3f target{ 1.0f, 1.0f, 1.0f };

    constexpr auto view = lookAt(origin, target, { 0.0f, 1.0f, 0.0f });
    constexpr auto inv_view = inverse(view);

    context_t context
    {
        .camera = {
            .inv_view = inv_view,
            .origin = origin,
        },
    };

    object_t data;

    if (std::ifstream stream("asset/plane.obj"); stream)
    {
        object_t plane;
        read_obj(stream, plane, { 0.9f, 0.9f, 0.9f }, 0.95f);
        data += scale(10.0f, 1.0f, 10.0f) * translation(-0.5f, 0.0f, -0.5f) * plane;
    }

    if (std::ifstream stream("asset/cube.obj"); stream)
    {
        object_t cube;
        read_obj(stream, cube, { 0.8f, 0.2f, 0.1f }, 0.8f);
        data += translation(-1.0f, 0.01f, 2.3f) * cube;
    }

    if (std::ifstream stream("asset/cube.obj"); stream)
    {
        object_t cube;
        read_obj(stream, cube, { 0.3f, 0.8f, 0.1f }, 0.05f);
        data += translation(2.9f, 0.01f, 2.6f) * cube;
    }

    if (std::ifstream stream("asset/cube.obj"); stream)
    {
        object_t cube;
        read_obj(stream, cube, { 0.3f, 0.1f, 0.8f }, 0.5f);
        data += translation(2.6f, 0.01f, -1.2f) * cube;
    }

    if (std::ifstream stream("asset/teapot.obj"); stream)
    {
        object_t teapot;
        read_obj(stream, teapot, { 0.8f, 0.8f, 0.8f }, 0.2f);
        data += translation(0.0f, 0.01f, 0.0f) * teapot;
    }

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
    glfwWindowHint(GLFW_OPENGL_COMPAT_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE);

    const auto window = glfwCreateWindow(
        800,
        600,
        "GLRT",
        nullptr,
        nullptr);
    if (!window)
        return 1;

    glfwSetWindowUserPointer(window, &context);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSwapInterval(1);
    glfwMakeContextCurrent(window);

    glewInit();

    glDebugMessageCallback(debug_callback, &context);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glCreateVertexArrays(1, &context.vertex_array);
    glCreateBuffers(sizeof(context.buffers) / sizeof(GLuint), context.buffers);

    glNamedBufferData(
        context.buffers[1],
        static_cast<GLsizeiptr>(data.indices.size() * sizeof(std::uint32_t)),
        data.indices.data(),
        GL_STATIC_DRAW);
    glNamedBufferData(
        context.buffers[2],
        static_cast<GLsizeiptr>(data.vertices.size() * sizeof(vertex_t)),
        data.vertices.data(),
        GL_STATIC_DRAW);
    glNamedBufferData(
        context.buffers[3],
        static_cast<GLsizeiptr>(bvh_nodes.size() * sizeof(bvh_node_t)),
        bvh_nodes.data(),
        GL_STATIC_DRAW);
    glNamedBufferData(
        context.buffers[4],
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

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, context.buffers[0]);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, context.buffers[1]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, context.buffers[2]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, context.buffers[3]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, context.buffers[4]);

    glfwShowWindow(window);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    framebuffer_size_callback(window, width, height);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glNamedBufferData(
            context.buffers[0],
            sizeof(camera_t),
            &context.camera,
            GL_STATIC_DRAW);

        context.camera.frame++;

        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(context.vertex_array);
        glUseProgram(program);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glUseProgram(0);

        glfwSwapBuffers(window);
    }

    glDeleteProgram(program);

    glDeleteVertexArrays(1, &context.vertex_array);
    glDeleteBuffers(sizeof(context.buffers) / sizeof(GLuint), context.buffers);
    glDeleteTextures(1, &context.accumulation);

    glfwDestroyWindow(window);
    glfwTerminate();
}
