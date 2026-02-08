#version 450 core

layout (location = 0) out vec4 color;

layout (row_major, binding = 0) uniform data_buffer {
    mat4 inv_view;
    mat4 inv_proj;
    vec3 origin;
    float total_light_area;
    uvec3 extent;
    uint frame;
    uvec2 tile_extent;
} data;

layout (rgba32f, binding = 0) uniform image2D accumulation;

void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec3 samples = imageLoad(accumulation, pixel).rgb;

    uvec2 tile_count = (data.extent.xy + data.tile_extent - 1u) / data.tile_extent;
    uint total_tile_count = tile_count.x * tile_count.y;
    uint sample_index = data.frame / total_tile_count;

    if (sample_index >= data.extent.z) {
        color = vec4(sqrt(samples / (float(data.extent.z) + 1.0)), 1.0);
        return;
    }

    color = vec4(sqrt(samples / (float(sample_index) + 1.0)), 1.0);
}
