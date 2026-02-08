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

struct uniform_data_t
{
    mat4f inv_view;
    mat4f inv_proj;
    vec3f origin;
    float total_light_area{};
    vec3u extent;
    std::uint32_t frame{};
};

struct context_t
{
    uniform_data_t data;

    gl::VertexArray vertex_array;
    gl::Texture accumulation;

    gl::Buffer data_buffer;
    gl::Buffer index_buffer;
    gl::Buffer vertex_buffer;
    gl::Buffer material_buffer;
    gl::Buffer node_buffer;
    gl::Buffer map_buffer;
    gl::Buffer light_buffer;
    gl::Buffer light_area_buffer;
};

static void framebuffer_size_callback(GLFWwindow *window, const int width, const int height)
{
    const auto context = static_cast<context_t *>(glfwGetWindowUserPointer(window));

    context->data.extent = {
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height),
        2500u,
    };
    context->data.frame = {};

    const auto proj = perspective(45.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
    context->data.inv_proj = inverse(proj);

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

static int load_shader(const GLuint program, const std::filesystem::path &path, const GLenum type)
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

static int link_program(const GLuint program)
{
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status)
    {
        char buf[1024];
        GLsizei len;
        glGetProgramInfoLog(program, sizeof(buf), &len, buf);
        buf[len] = 0;
        std::cerr << buf << std::endl;
        return 1;
    }

    return 0;
}

static int validate_program(const GLuint program)
{
    glValidateProgram(program);

    GLint status;
    glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
    if (!status)
    {
        char buf[1024];
        GLsizei len;
        glGetProgramInfoLog(program, sizeof(buf), &len, buf);
        buf[len] = 0;
        std::cerr << buf << std::endl;
        return 1;
    }

    return 0;
}

static void generate_scene(model_t &data)
{
    {
        model_t cornell;
        read_obj("asset/cornell.obj", cornell);
        data += scale(4.0f, 4.0f, 4.0f) * cornell;
    }

    {
        model_t teapot;
        read_obj("asset/teapot.obj", teapot);
        data += translation(0.0f, -4.0f, 0.0f) * teapot;
    }
}

static void generate_scene1(model_t &data)
{
    {
        model_t plane;
        read_obj("asset/plane.obj", plane);
        data += scale(10.0f, 1.0f, 10.0f) * translation(-0.5f, 0.0f, -0.5f) * plane;
    }

    {
        model_t cube;
        read_obj("asset/cube.obj", cube);
        data += translation(-1.0f, 0.01f, 2.3f) * cube;
    }

    {
        model_t cube;
        read_obj("asset/cube.obj", cube);
        data += translation(2.9f, 0.01f, 2.6f) * cube;
    }

    {
        model_t cube;
        read_obj("asset/cube.obj", cube);
        data += translation(2.6f, 0.01f, -1.2f) * cube;
    }

    {
        model_t teapot;
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

    model_t data;
    generate_scene(data);

    bvh_t bvh;
    build_bvh(data, bvh);

    const Window window;

    context_t context
    {
        .data = {
            .inv_view = inv_view,
            .origin = origin,
            .total_light_area = bvh.total_light_area,
        },
        .accumulation = gl::Texture(GL_TEXTURE_2D),
    };

    window.SetUserPointer(&context);
    window.SetFramebufferSizeCallback(framebuffer_size_callback);

    glDebugMessageCallback(debug_callback, &context);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    context.data_buffer.Bind(GL_UNIFORM_BUFFER, 0);

    context.index_buffer.Bind(GL_SHADER_STORAGE_BUFFER, 1);
    context.vertex_buffer.Bind(GL_SHADER_STORAGE_BUFFER, 2);
    context.material_buffer.Bind(GL_SHADER_STORAGE_BUFFER, 3);
    context.node_buffer.Bind(GL_SHADER_STORAGE_BUFFER, 4);
    context.map_buffer.Bind(GL_SHADER_STORAGE_BUFFER, 5);
    context.light_buffer.Bind(GL_SHADER_STORAGE_BUFFER, 6);
    context.light_area_buffer.Bind(GL_SHADER_STORAGE_BUFFER, 7);

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
    context.light_buffer.Data(
        bvh.lights.data(),
        bvh.lights.size() * sizeof(std::uint32_t),
        GL_STATIC_DRAW);
    context.light_area_buffer.Data(
        bvh.light_areas.data(),
        bvh.light_areas.size() * sizeof(std::uint32_t),
        GL_STATIC_DRAW);

    const auto program = glCreateProgram();

    if (const auto error = load_shader(program, "asset/shader/default.vert", GL_VERTEX_SHADER))
        return error;
    if (const auto error = load_shader(program, "asset/shader/default.frag", GL_FRAGMENT_SHADER))
        return error;

    if (const auto error = link_program(program))
        return error;
    if (const auto error = validate_program(program))
        return error;

    const auto compute_program = glCreateProgram();

    if (const auto error = load_shader(compute_program, "asset/shader/default.glsl", GL_COMPUTE_SHADER))
        return error;

    if (const auto error = link_program(compute_program))
        return error;
    if (const auto error = validate_program(compute_program))
        return error;

    window.Show();

    int width, height;
    window.GetFramebufferSize(width, height);
    framebuffer_size_callback(window.GetHandle(), width, height);

    while (!window.ShouldClose())
    {
        glfwPollEvents();

        window.GetFramebufferSize(width, height);

        context.data_buffer.Data(
            &context.data,
            sizeof(uniform_data_t),
            GL_STATIC_DRAW);

        context.data.frame++;

        glClear(GL_COLOR_BUFFER_BIT);

        context.vertex_array.Bind();
        glUseProgram(program);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        window.SwapBuffers();
    }

    glDeleteProgram(program);
}
