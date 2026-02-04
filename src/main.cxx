#include <filesystem>
#include <fstream>
#include <iostream>
#include <istream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glrt/bvh.hxx>
#include <glrt/gl.hxx>
#include <glrt/math.hxx>
#include <glrt/obj.hxx>
#include <glrt/window.hxx>

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

    gl::VertexArray vertex_array;
    gl::Texture accumulation;

    gl::Buffer camera_buffer;
    gl::Buffer index_buffer;
    gl::Buffer vertex_buffer;
    gl::Buffer material_buffer;
    gl::Buffer node_buffer;
    gl::Buffer map_buffer;
};

static void framebuffer_size_callback(GLFWwindow *window, const int width, const int height)
{
    const auto context = static_cast<context_t *>(glfwGetWindowUserPointer(window));

    context->camera.frame = {};

    const auto proj = perspective(45.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
    context->camera.inv_proj = inverse(proj);

    context->accumulation.Recreate(GL_TEXTURE_2D);
    context->accumulation.Storage2D(1, GL_RGBA32F, width, height);
    context->accumulation.BindImage(0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);

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

static int load_shader(const std::filesystem::path &path, const GLenum type, const GLuint program)
{
    std::vector<char> source;
    if (std::ifstream stream(path, std::istream::ate); stream)
    {
        source.resize(stream.tellg());
        stream.seekg(0, std::ios::beg);
        stream.read(source.data(), static_cast<std::streamsize>(source.size()));
    }

    const auto shader = glCreateShader(type);
    const auto data_ptr = source.data();
    const auto length = static_cast<GLint>(source.size());

    glShaderSource(shader, 1, &data_ptr, &length);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        char buf[1024];
        GLsizei len;
        glGetShaderInfoLog(shader, sizeof(buf), &len, buf);
        buf[len] = 0;
        std::cerr << buf << std::endl;
        return 1;
    }

    glAttachShader(program, shader);
    glDeleteShader(shader);

    return 0;
}

static void generate_scene(object_t &data)
{
    {
        object_t cornell;
        read_obj("asset/cornell.obj", cornell);
        data += scale(4.0f, 4.0f, 4.0f) * cornell;
    }

    {
        object_t teapot;
        read_obj("asset/teapot.obj", teapot);
        data += translation(0.0f, -4.0f, 0.0f) * teapot;
    }
}

static void generate_scene1(object_t &data)
{
    {
        object_t plane;
        read_obj("asset/plane.obj", plane);
        data += scale(10.0f, 1.0f, 10.0f) * translation(-0.5f, 0.0f, -0.5f) * plane;
    }

    {
        object_t cube;
        read_obj("asset/cube.obj", cube);
        data += translation(-1.0f, 0.01f, 2.3f) * cube;
    }

    {
        object_t cube;
        read_obj("asset/cube.obj", cube);
        data += translation(2.9f, 0.01f, 2.6f) * cube;
    }

    {
        object_t cube;
        read_obj("asset/cube.obj", cube);
        data += translation(2.6f, 0.01f, -1.2f) * cube;
    }

    {
        object_t teapot;
        read_obj("asset/teapot.obj", teapot);
        data += translation(0.0f, 0.01f, 0.0f) * teapot;
    }
}

int main()
{
    constexpr vec3f origin{ 0.0f, 0.0f, 14.0f };
    constexpr vec3f target{ 0.0f, 0.0f, 0.0f };

    constexpr auto view = lookAt(origin, target, { 0.0f, 1.0f, 0.0f });
    constexpr auto inv_view = inverse(view);

    object_t data;
    generate_scene(data);

    bvh_t bvh;
    build_bvh(data, bvh);

    const Window window;

    context_t context
    {
        .camera = {
            .inv_view = inv_view,
            .origin = origin,
        },
        .accumulation = gl::Texture(GL_TEXTURE_2D),
    };

    window.SetUserPointer(&context);
    window.SetFramebufferSizeCallback(framebuffer_size_callback);

    glDebugMessageCallback(debug_callback, &context);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    context.camera_buffer.Bind(GL_UNIFORM_BUFFER, 0);

    context.index_buffer.Bind(GL_SHADER_STORAGE_BUFFER, 1);
    context.vertex_buffer.Bind(GL_SHADER_STORAGE_BUFFER, 2);
    context.material_buffer.Bind(GL_SHADER_STORAGE_BUFFER, 3);
    context.node_buffer.Bind(GL_SHADER_STORAGE_BUFFER, 4);
    context.map_buffer.Bind(GL_SHADER_STORAGE_BUFFER, 5);

    context.index_buffer.Data(
        data.indices.data(),
        data.indices.size() * sizeof(std::uint32_t),
        GL_STATIC_DRAW);
    context.vertex_buffer.Data(
        data.vertices.data(),
        data.vertices.size() * sizeof(vertex_t),
        GL_STATIC_DRAW);
    context.material_buffer.Data(
        data.materials.data(),
        data.materials.size() * sizeof(material_t),
        GL_STATIC_DRAW);
    context.node_buffer.Data(
        bvh.nodes.data(),
        bvh.nodes.size() * sizeof(bvh_node_t),
        GL_STATIC_DRAW);
    context.map_buffer.Data(
        bvh.map.data(),
        bvh.map.size() * sizeof(std::uint32_t),
        GL_STATIC_DRAW);

    const auto program = glCreateProgram();

    if (const auto error = load_shader("asset/default.vert", GL_VERTEX_SHADER, program))
        return error;

    if (const auto error = load_shader("asset/default.frag", GL_FRAGMENT_SHADER, program))
        return error;

    glLinkProgram(program);
    glValidateProgram(program);

    window.Show();

    int width, height;
    window.GetFramebufferSize(width, height);
    framebuffer_size_callback(window.GetHandle(), width, height);

    while (!window.ShouldClose())
    {
        glfwPollEvents();

        context.camera_buffer.Data(
            &context.camera,
            sizeof(camera_t),
            GL_STATIC_DRAW);

        context.camera.frame++;

        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        context.vertex_array.Bind();
        glUseProgram(program);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        window.SwapBuffers();
    }

    glDeleteProgram(program);
}
