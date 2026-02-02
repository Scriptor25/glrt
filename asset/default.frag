#version 450 core

struct ray_t {
    vec3 origin;
    vec3 direction;
};

struct record_t {
    float t;
    vec3 position;
    vec3 normal;
    vec2 texture;
};

struct bvh_node_t {
    vec3 box_min;
    uint left;
    vec3 box_max;
    uint right;
};

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 color;

layout (binding = 1) uniform camera_buffer {
    mat4 inv_view;
    mat4 inv_proj;
    vec3 origin;
} camera;

layout (std430, binding = 1) buffer index_buffer {
    uint indices[];
};
layout (std430, binding = 2) buffer position_buffer {
    vec3 positions[];
};
layout (std430, binding = 3) buffer normal_buffer {
    vec3 normals[];
};
layout (std430, binding = 4) buffer texture_buffer {
    vec2 textures[];
};
layout (std430, binding = 5) buffer bvh_node_buffer {
    bvh_node_t nodes[];
};

const float EPSILON = 1e-4;
const float PI = 3.14159265359;

const uint MAX_BOUNCES = 50u;

vec3 ray_at(in ray_t self, in float t) {
    return self.origin + self.direction * t;
}

uint hash(uint x) {
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

float rand(inout uint seed) {
    seed = hash(seed);
    return float(seed) / float(0xffffffffu);
}

vec3 random_unit_vector(inout uint seed) {
    float z = rand(seed) * 2.0 - 1.0;
    float a = rand(seed) * 2.0 * PI;
    float r = sqrt(max(0.0, 1.0 - z * z));
    return vec3(r * cos(a), r * sin(a), z);
}

bool hit_box(in ray_t ray, in vec3 box_min, in vec3 box_max, in float t_max) {

    vec3 inv_d = 1.0 / ray.direction;

    vec3 t0 = (box_min - ray.origin) * inv_d;
    vec3 t1 = (box_max - ray.origin) * inv_d;

    vec3 t_min_v = min(t0, t1);
    vec3 t_max_v = max(t0, t1);

    float t_enter = max(max(t_min_v.x, t_min_v.y), t_min_v.z);
    float t_exit  = min(min(t_max_v.x, t_max_v.y), t_max_v.z);

    return t_exit >= max(t_enter, 0.0) && t_enter < t_max;
}

bool hit_triangle(in ray_t ray, in uint tri, inout record_t rec) {

    uint i0 = indices[tri * 3 + 0];
    uint i1 = indices[tri * 3 + 1];
    uint i2 = indices[tri * 3 + 2];

    vec3 p0 = positions[i0];
    vec3 p1 = positions[i1];
    vec3 p2 = positions[i2];

    vec3 e1 = p1 - p0;
    vec3 e2 = p2 - p0;

    vec3 p = cross(ray.direction, e2);
    float det = dot(e1, p);

    if (abs(det) < EPSILON)
        return false;

    float inv_det = 1.0 / det;

    vec3 s = ray.origin - p0;
    float u = inv_det * dot(s, p);
    if (u < 0.0 || u > 1.0)
        return false;

    vec3 q = cross(s, e1);
    float v = inv_det * dot(ray.direction, q);
    if (v < 0.0 || u + v > 1.0)
        return false;

    float t = inv_det * dot(e2, q);
    if (t < EPSILON || t >= rec.t)
        return false;

    rec.t = t;
    rec.position = ray_at(ray, t);

    float w = 1.0 - u - v;

    vec3 n0 = normals[i0];
    vec3 n1 = normals[i1];
    vec3 n2 = normals[i2];

    rec.normal = normalize(
        n0 * w +
        n1 * u +
        n2 * v
    );

    vec2 t0 = textures[i0];
    vec2 t1 = textures[i1];
    vec2 t2 = textures[i2];

    rec.texture =
        t0 * w +
        t1 * u +
        t2 * v;

    return true;
}

bool hit(in ray_t ray, inout record_t rec) {

    bool hit_anything = false;

    uint stack[64];
    int stack_ptr = 0;

    stack[stack_ptr++] = 0u;

    while (stack_ptr > 0) {

        uint node_index = stack[--stack_ptr];
        bvh_node_t node = nodes[node_index];

        if (!hit_box(ray, node.box_min, node.box_max, rec.t))
            continue;

        if (node.right <= 4u) {
            uint first = node.left;
            uint count = node.right;

            for (uint i = 0u; i < count; ++i) {
                if (hit_triangle(ray, first + i * 3u, rec))
                    hit_anything = true;
            }
        }
        else {
            stack[stack_ptr++] = node.left;
            stack[stack_ptr++] = node.right;
        }
    }

    return hit_anything;
}

void scatter(inout ray_t ray, in record_t rec) {
    vec3 N = faceforward(rec.normal, -ray.direction, rec.normal);

    ray.origin = rec.position + N * EPSILON;
    ray.direction = normalize(reflect(ray.direction, N)/* + random_unit_vector(seed)*/);
}

vec3 miss(in ray_t ray) {
    float t = 0.5 * (ray.direction.y + 1.0);
    return mix(vec3(0.8, 0.9, 1.0), vec3(0.2, 0.4, 0.8), t);
}

const uint frame_index = 1u;

void main() {
    uint seed =
        uint(gl_FragCoord.x) * 1973u ^
        uint(gl_FragCoord.y) * 9277u ^
        frame_index * 26699u;

    vec3 radiance   = vec3(0.0);
    vec3 throughput = vec3(1.0);

    vec2 ndc = uv * 2.0 - 1.0;

    vec4 clip = vec4(ndc, -1.0, 1.0);
    vec4 view = camera.inv_proj * clip;
    view /= view.w;

    vec3 direction = normalize((camera.inv_view * vec4(view.xyz, 0.0)).xyz);

    ray_t ray;
    ray.origin    = camera.origin;
    ray.direction = direction;

    record_t rec;
    for (uint bounce = 0u; bounce < MAX_BOUNCES; ++bounce) {

        rec.t = 1e30;

        if (!hit(ray, rec)) {
            radiance += throughput * miss(ray);
            break;
        }

        throughput *= vec3(0.8);

        scatter(ray, rec);
    }

    color = vec4(radiance, 1.0);
}
