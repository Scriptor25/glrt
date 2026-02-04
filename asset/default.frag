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
    uint material;
};

struct vertex_t {
    vec3 position;
    float _0;
    vec3 normal;
    float _1;
    vec2 texture;
    uint material;
    float _2;
};

struct material_t {
    vec3 albedo;
    float _0;
    vec3 emission;
    float roughness;
    float metallic;
    float sheen;
    float clearcoat_thickness;
    float clearcoat_roughness;
};

struct bvh_node_t {
    vec3 box_min;
    float _0;
    vec3 box_max;
    float _1;
    uint left;
    uint right;
    uint begin;
    uint end;
};

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 color;

layout (row_major, binding = 0) uniform camera_buffer {
    mat4 inv_view;
    mat4 inv_proj;
    vec3 origin;
    uint frame;
} camera;

layout(rgba32f, binding = 0) uniform image2D accumulation;

layout (std430, binding = 1) buffer index_buffer {
    uint indices[];
};
layout (std430, binding = 2) buffer vertex_buffer {
    vertex_t vertices[];
};
layout (std430, binding = 3) buffer material_buffer {
    material_t materials[];
};
layout (std430, binding = 4) buffer node_buffer {
    bvh_node_t nodes[];
};
layout (std430, binding = 5) buffer map_buffer {
    uint bvh_map[];
};

const float EPSILON = 1e-4;
const float PI = 3.14159265359;

vec3 ray_at(in ray_t self, in float t) {
    return self.origin + self.direction * t;
}

uint seed = 0u;

uint hash(uint x) {
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

float rand() {
    seed = hash(seed);
    return float(seed) / float(0xffffffffu);
}

vec3 random_unit_vector() {
    float z = rand() * 2.0 - 1.0;
    float a = rand() * 2.0 * PI;
    float r = sqrt(max(0.0, 1.0 - z * z));
    return vec3(r * cos(a), r * sin(a), z);
}

mat3 make_tbn(vec3 n) {
    vec3 t = normalize(abs(n.z) < 0.999
        ? cross(n, vec3(0, 0, 1))
        : cross(n, vec3(0, 1, 0)));

    vec3 b = cross(n, t);
    return mat3(t, b, n);
}

vec3 fresnel_schlick(float cos_theta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec3 fresnel_sheen(float cos_theta, vec3 sheen_color) {
    return sheen_color + (1.0 - sheen_color) * pow(1.0 - cos_theta, 5.0);
}

float D_GGX(float NoH, float alpha) {
    float a2 = alpha * alpha;
    float d = NoH * NoH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

float D_GGX_Clearcoat(float NoH, float roughness) {
    float a = mix(0.001, 0.1, roughness);
    float a2 = a * a;
    float d = NoH * NoH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

float D_Charlie(float NoH, float roughness) {
    float alpha = max(roughness * roughness, 0.001);
    float inv_alpha = 1.0 / alpha;
    float sin2 = max(1.0 - NoH * NoH, 0.0);
    return (2.0 + inv_alpha) * pow(sin2, inv_alpha * 0.5) / (2.0 * PI);
}

float G_SchlickGGX(float NoV, float k) {
    return NoV / (NoV * (1.0 - k) + k);
}

float G_Smith(float NoV, float NoL, float roughness) {
    float k = (roughness + 1.0);
    k = (k * k) / 8.0;
    return G_SchlickGGX(NoV, k) * G_SchlickGGX(NoL, k);
}

float G_Clearcoat(float NoV, float NoL) {
    return G_SchlickGGX(NoV, 0.25) * G_SchlickGGX(NoL, 0.25);
}

vec3 sample_GGX(vec2 xi, float roughness) {
    float a = roughness * roughness;

    float phi = 2.0 * PI * xi.x;
    float cos_theta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    return vec3(
        sin_theta * cos(phi),
        sin_theta * sin(phi),
        cos_theta
    );
}

vec3 sample_clearcoat(vec2 xi, float roughness) {
    float a = mix(0.001, 0.1, roughness);

    float phi = 2.0 * PI * xi.x;
    float cos_theta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    return vec3(
        sin_theta * cos(phi),
        sin_theta * sin(phi),
        cos_theta
    );
}

float pdf_GGX(vec3 N, vec3 H, vec3 V, float roughness) {
    float NoH = max(dot(N, H), 0.0);
    float VoH = max(dot(V, H), 0.0);

    float D = D_GGX(NoH, roughness * roughness);
    return (D * NoH) / (4.0 * VoH + 1e-5);
}

vec3 sample_cosine_hemisphere() {
    float u1 = rand();
    float u2 = rand();

    float r = sqrt(u1);
    float phi = 2.0 * PI * u2;

    return vec3(
        r * cos(phi),
        r * sin(phi),
        sqrt(1.0 - u1)
    );
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

bool hit_triangle(in ray_t ray, in uint base, inout record_t rec) {

    uint i0 = indices[base + 0];
    uint i1 = indices[base + 1];
    uint i2 = indices[base + 2];

    vec3 p0 = vertices[i0].position;
    vec3 p1 = vertices[i1].position;
    vec3 p2 = vertices[i2].position;

    vec3 e1 = p1 - p0;
    vec3 e2 = p2 - p0;

    vec3 p = cross(ray.direction, e2);
    float det = dot(e1, p);

    if (/* abs */(det) < EPSILON)
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

    vec3 n0 = vertices[i0].normal;
    vec3 n1 = vertices[i1].normal;
    vec3 n2 = vertices[i2].normal;

    vec3 nr = cross(e1, e2);
    vec3 n = normalize(
        n0 * w +
        n1 * u +
        n2 * v
    );

    rec.normal = faceforward(n, ray.direction, nr);

    vec2 t0 = vertices[i0].texture;
    vec2 t1 = vertices[i1].texture;
    vec2 t2 = vertices[i2].texture;

    rec.texture =
        t0 * w +
        t1 * u +
        t2 * v;

    uint m0 = vertices[i0].material;
    uint m1 = vertices[i1].material;
    uint m2 = vertices[i2].material;

    rec.material = max(m0, max(m1, m2)); // TODO: material interpolation

    return true;
}

bool hit(in ray_t ray, inout record_t rec) {

    bool hit_anything = false;

    for (uint i = 0u; i < indices.length(); i += 3) {
        if (hit_triangle(ray, i, rec)) {
            hit_anything = true;
        }
    }

    return hit_anything;
}

bool hit_bvh(in ray_t ray, inout record_t rec) {

    bool hit_anything = false;

    uint stack[64];
    int stack_ptr = 0;

    stack[stack_ptr++] = 0u;

    while (stack_ptr > 0) {

        uint node_index = stack[--stack_ptr];
        bvh_node_t node = nodes[node_index];

        if (!hit_box(ray, node.box_min, node.box_max, rec.t))
            continue;

        if (node.begin != node.end) {
            for (uint i = node.begin; i < node.end; ++i) {
                if (hit_triangle(ray, bvh_map[i], rec)) {
                    hit_anything = true;
                }
            }
        }
        else if (stack_ptr + 2 < 64) {
            stack[stack_ptr++] = node.left;
            stack[stack_ptr++] = node.right;
        }
    }

    return hit_anything;
}

bool scatter(inout ray_t ray, in record_t rec, inout vec3 throughput, inout vec3 radiance) {
    if (rec.material >= materials.length())
        return false;

    material_t mat = materials[rec.material];

    if (length(mat.emission) > 0.0) {
        radiance += throughput * mat.emission;
        return false;
    }

    vec3 N = rec.normal;
    vec3 V = normalize(-ray.direction);
    float NoV = max(dot(N, V), 0.0);
    if (NoV <= 0.0) return false;

    mat3 tbn = make_tbn(N);

    float w_diffuse  = (1.0 - mat.metallic);
    float w_specular = 1.0;
    float w_clear    = mat.clearcoat_thickness * 0.25;

    float sum = w_diffuse + w_specular + w_clear;
    w_diffuse  /= sum;
    w_specular /= sum;
    w_clear    /= sum;

    float r = rand();

    vec3 L;
    float pdf;
    vec3 brdf;

    vec3 F0 = mix(vec3(0.04), mat.albedo, mat.metallic);

    if (r < w_specular) {
        vec2 xi = vec2(rand(), rand());
        vec3 H = normalize(tbn * sample_GGX(xi, max(0.001, mat.roughness)));
        L = reflect(-V, H);

        float NoL = max(dot(N, L), 0.0);
        if (NoL <= 0.0) return false;

        float NoH = max(dot(N, H), 0.0);
        float VoH = max(dot(V, H), 0.0);

        float D = D_GGX(NoH, mat.roughness * mat.roughness);
        float G = G_Smith(NoV, NoL, mat.roughness);
        vec3  F = fresnel_schlick(VoH, F0);

        brdf = (D * G * F) / max(4.0 * NoV * NoL, 1e-5);
        pdf  = pdf_GGX(N, H, V, mat.roughness);

        throughput *= brdf * NoL / (pdf * w_specular);
    }
    else if (r < w_specular + w_clear) {
        vec2 xi = vec2(rand(), rand());
        vec3 H = normalize(tbn * sample_clearcoat(xi, mat.clearcoat_roughness));
        L = reflect(-V, H);

        float NoL = max(dot(N, L), 0.0);
        if (NoL <= 0.0) return false;

        float NoH = max(dot(N, H), 0.0);
        float VoH = max(dot(V, H), 0.0);

        float D = D_GGX_Clearcoat(NoH, mat.clearcoat_roughness);
        float G = G_Clearcoat(NoV, NoL);
        vec3  F = vec3(0.04);

        brdf = (D * G * F) / max(4.0 * NoV * NoL, 1e-5);
        pdf  = (D * NoH) / max(4.0 * VoH, 1e-5);

        throughput *= brdf * NoL / (pdf * w_clear);
    }
    else {
        vec3 L_local = sample_cosine_hemisphere();
        L = normalize(tbn * L_local);

        float NoL = max(dot(N, L), 0.0);
        if (NoL <= 0.0) return false;

        vec3 F = fresnel_schlick(NoV, F0);
        vec3 kd = (1.0 - F) * (1.0 - mat.metallic);

        brdf = kd * mat.albedo / PI;
        pdf  = NoL / PI;

        if (mat.sheen > 0.0) {
            float NoH = max(dot(N, normalize(V + L)), 0.0);
            float D = D_Charlie(NoH, mat.roughness);
            vec3  Fs = fresnel_sheen(NoV, mat.albedo);
            brdf += mat.sheen * D * Fs;
        }

        throughput *= brdf * NoL / (pdf * w_diffuse);
    }

    ray.origin    = rec.position + N * EPSILON;
    ray.direction = normalize(L);

    return true;
}

vec3 miss(in ray_t ray) {
    float t = 0.5 * (ray.direction.y + 1.0);
    vec3 sky = mix(vec3(0.8, 0.9, 1.0), vec3(0.2, 0.4, 0.8), t);

    vec3 sun_direction = normalize(vec3(-0.1, 0.7, 0.5));
    float sun_dot = max(dot(ray.direction, sun_direction), 0.0);

    float sun_disk = pow(sun_dot, 2000.0);

    float sun_glow = pow(sun_dot, 50.0);

    vec3 sun_color = vec3(1.0, 0.95, 0.8);

    vec3 color = sky;
    color += sun_color * sun_disk * 20.0;
    color += sun_color * sun_glow * 0.5;

    return color;
}

void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec4 samples = imageLoad(accumulation, pixel);

    if (camera.frame >= 2000) {
        color = samples / 2000;
        return;
    }

    seed =
        uint(pixel.x) * 1973u ^
        uint(pixel.y) * 9277u ^
        camera.frame * 26699u;

    vec3 radiance   = vec3(0.0);
    vec3 throughput = vec3(1.0);

    vec2 delta = 1.0 / imageSize(accumulation);
    vec2 offset = vec2(delta.x * rand(), delta.y * rand());

    vec2 ndc = (uv + offset) * 2.0 - 1.0;

    vec4 clip = vec4(ndc, -1.0, 1.0);
    vec4 view = camera.inv_proj * clip;
    view /= view.w;

    vec3 direction = normalize((camera.inv_view * vec4(view.xyz, 0.0)).xyz);

    ray_t ray;
    ray.origin    = camera.origin;
    ray.direction = direction;

    record_t rec;
    for (uint bounce = 0u; bounce < 3u; ++bounce) {

        rec.t = 1e30;

        if (!hit_bvh(ray, rec)) {
            // radiance += throughput * miss(ray);
            break;
        }

        if (!scatter(ray, rec, throughput, radiance)) {
            break;
        }
    }

    samples = vec4(samples.rgb + radiance, 1.0);
    imageStore(accumulation, pixel, samples);

    color = vec4(samples.rgb / (float(camera.frame) + 1.0), 1.0);
}
