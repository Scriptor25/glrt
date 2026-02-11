#include <filesystem>
#include <fstream>
#include <iostream>
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
    vec2u tile_extent;
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

    gl::Program draw_program;
    gl::Program compute_program;
};

static void framebuffer_size_callback(GLFWwindow *window, const int width, const int height)
{
    const auto context = static_cast<context_t *>(glfwGetWindowUserPointer(window));

    context->data.extent = {
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height),
        1600u,
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
    GLenum /*source*/,
    GLenum /*type*/,
    GLuint /*id*/,
    GLenum /*severity*/,
    GLsizei /*length*/,
    const GLchar *message,
    const void *userParam)
{
    // auto context = static_cast<const context_t *>(userParam);

    std::cerr << message << std::endl;
}

static void generate_scene(model_t &data)
{
    {
        model_t cornell;
        read_obj("asset/model/cornell/cornell.obj", cornell);
        data += scale(4.0f, 4.0f, 4.0f) * cornell;
    }

    {
        model_t teapot;
        read_obj("asset/model/teapot/teapot.obj", teapot);
        data += translation(0.0f, -4.0f, 0.0f) * teapot;
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
            .tile_extent = { 64u, 64u },
        },
        .accumulation = gl::Texture(GL_TEXTURE_2D),
    };

    gl::Error error;

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

    if (context.draw_program.LoadShaderBinary(
        "asset/shader/default.vert.spv",
        GL_VERTEX_SHADER,
        GL_SHADER_BINARY_FORMAT_SPIR_V,
        error); error)
    {
        std::cerr << error.message() << std::endl;
        return error.code();
    }

    if (context.draw_program.LoadShaderBinary(
        "asset/shader/default.frag.spv",
        GL_FRAGMENT_SHADER,
        GL_SHADER_BINARY_FORMAT_SPIR_V,
        error); error)
    {
        std::cerr << error.message() << std::endl;
        return error.code();
    }

    if (context.draw_program.Link(error); error)
    {
        std::cerr << error.message() << std::endl;
        return error.code();
    }

    if (context.draw_program.Validate(error); error)
    {
        std::cerr << error.message() << std::endl;
        return error.code();
    }

    if (context.compute_program.LoadShaderBinary(
        "asset/shader/default.comp.spv",
        GL_COMPUTE_SHADER,
        GL_SHADER_BINARY_FORMAT_SPIR_V,
        error); error)
    {
        std::cerr << error.message() << std::endl;
        return error.code();
    }

    if (context.compute_program.Link(error); error)
    {
        std::cerr << error.message() << std::endl;
        return error.code();
    }

    if (context.compute_program.Validate(error); error)
    {
        std::cerr << error.message() << std::endl;
        return error.code();
    }

    window.Show();

    int width, height;
    window.GetFramebufferSize(width, height);
    framebuffer_size_callback(window.GetHandle(), width, height);

    while (!window.ShouldClose())
    {
        glfwPollEvents();

        context.compute_program.Bind();

        context.data_buffer.Data(
            &context.data,
            sizeof(uniform_data_t),
            GL_STATIC_DRAW);

        auto tile_count = (vec2u(context.data.extent.swizzle<0, 1>()) + context.data.tile_extent - 1u)
                          / context.data.tile_extent;
        auto total_tile_count = tile_count[0] * tile_count[1];
        auto sample_index = context.data.frame / total_tile_count;

        context.data.frame++;

        if (sample_index < context.data.extent[2])
        {
            auto groups = (context.data.tile_extent + 7u) / 8u;

            glDispatchCompute(
                groups[0],
                groups[1],
                1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

        context.vertex_array.Bind();
        context.draw_program.Bind();

        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        window.SwapBuffers();
    }
}
