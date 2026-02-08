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

layout (row_major, binding = 0) uniform data_buffer {
    mat4 inv_view;
    mat4 inv_proj;
    vec3 origin;
    uint frame;
    float total_light_area;
} data;

layout (rgba32f, binding = 0) uniform image2D accumulation;

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

layout (std430, binding = 6) buffer light_buffer {
    uint light_triangles[];
};

layout (std430, binding = 7) buffer light_area_buffer {
    float light_areas[];
};

/* constant */

const float EPSILON = 1e-5;
const float PI = 3.14159265359;

const uint MAX_SAMPLES_ROOT = 50;
const uint MAX_SAMPLES = MAX_SAMPLES_ROOT * MAX_SAMPLES_ROOT;
const float INV_MAX_SAMPLES_ROOT = 1.0 / float(MAX_SAMPLES_ROOT);

const uint TILE_W = 100u;
const uint TILE_H = 100u;

/* ray */

vec3 ray_at(in ray_t self, in float t) {
    return self.origin + self.direction * t;
}

/* random */

uint seed = 0u;

uint hash(in uint x) {
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

float random() {
    seed = hash(seed);
    return float(seed) / float(0xffffffffu);
}

mat3 make_tbn(in vec3 n) {
    vec3 v = mix(vec3(0, 0, 1), vec3(0, 1, 0), step(0.999, abs(n.z)));
    vec3 t = normalize(cross(n, v));

    vec3 b = cross(n, t);
    return mat3(t, b, n);
}

/* hit */

bool hit_box(in ray_t ray, in vec3 box_min, in vec3 box_max, in float t_max) {

    vec3 inv_d = 1.0 / ray.direction;

    vec3 t0 = (box_min - ray.origin) * inv_d;
    vec3 t1 = (box_max - ray.origin) * inv_d;

    vec3 t_min_v = min(t0, t1);
    vec3 t_max_v = max(t0, t1);

    float t_enter = max(max(t_min_v.x, t_min_v.y), t_min_v.z);
    float t_exit = min(min(t_max_v.x, t_max_v.y), t_max_v.z);

    return t_exit >= max(t_enter, 0.0) && t_enter < t_max;
}

bool hit_triangle(in ray_t ray, in bool test, in uint base, inout record_t rec) {

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

    if (/* abs */(det) < EPSILON) {
        return false;
    }

    float inv_det = 1.0 / det;

    vec3 s = ray.origin - p0;
    float u = inv_det * dot(s, p);
    if (u < 0.0 || u > 1.0) {
        return false;
    }

    vec3 q = cross(s, e1);
    float v = inv_det * dot(ray.direction, q);
    if (v < 0.0 || u + v > 1.0) {
        return false;
    }

    float t = inv_det * dot(e2, q);
    if (t < EPSILON || t >= rec.t) {
        return false;
    }

    if (test) {
        return true;
    }

    rec.t = t;
    rec.position = ray_at(ray, t);

    float w = 1.0 - u - v;

    vec3 n0 = vertices[i0].normal;
    vec3 n1 = vertices[i1].normal;
    vec3 n2 = vertices[i2].normal;

    vec3 nr = cross(e1, e2);
    vec3 n = normalize(n0 * w + n1 * u + n2 * v);

    rec.normal = faceforward(n, ray.direction, nr);

    vec2 t0 = vertices[i0].texture;
    vec2 t1 = vertices[i1].texture;
    vec2 t2 = vertices[i2].texture;

    rec.texture = t0 * w + t1 * u + t2 * v;

    rec.material = vertices[i0].material;

    return true;
}

bool hit(in ray_t ray, in bool test, inout record_t rec) {

    bool hit_anything = false;

    for (uint i = 0u; i < indices.length(); i += 3) {
        if (hit_triangle(ray, test, i, rec)) {
            hit_anything = true;
        }
    }

    return hit_anything;
}

bool hit_bvh(in ray_t ray, in bool test, inout record_t rec) {

    bool hit_anything = false;

    uint stack[64];
    int stack_ptr = 0;

    stack[stack_ptr++] = 0u;

    while (stack_ptr > 0) {

        uint node_index = stack[--stack_ptr];
        bvh_node_t node = nodes[node_index];

        if (!hit_box(ray, node.box_min, node.box_max, rec.t)) {
            continue;
        }

        if (node.begin != node.end) {
            for (uint i = node.begin; i < node.end; ++i) {
                if (hit_triangle(ray, test, bvh_map[i], rec)) {
                    hit_anything = true;
                    if (test) {
                        return true;
                    }
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

/* fresnel */

vec3 fresnel_schlick(in float cos_theta, in vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec3 fresnel_sheen(in float cos_theta, in vec3 sheen_color) {
    return sheen_color + (1.0 - sheen_color) * pow(1.0 - cos_theta, 5.0);
}

/* D (distribution) */

float D_GGX(in float NoH, in float roughness) {
    float a2 = roughness * roughness;
    float d = NoH * NoH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

float D_GGX_Clearcoat(in float NoH, in float roughness) {
    float a = mix(0.001, 0.1, roughness);
    float a2 = a * a;
    float d = NoH * NoH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

float D_Charlie(in float NoH, in float roughness) {
    float alpha = max(roughness * roughness, 0.001);
    float inv_alpha = 1.0 / alpha;
    float sin2 = max(1.0 - NoH * NoH, 0.0);
    return (2.0 + inv_alpha) * pow(sin2, inv_alpha * 0.5) / (2.0 * PI);
}

/* G */

float G_SchlickGGX(in float NoV, in float k) {
    return NoV / (NoV * (1.0 - k) + k);
}

float G_Smith(in float NoV, in float NoL, in float roughness) {
    float k = (roughness + 1.0);
    k = (k * k) / 8.0;
    return G_SchlickGGX(NoV, k) * G_SchlickGGX(NoL, k);
}

float G_Clearcoat(in float NoV, in float NoL) {
    return G_SchlickGGX(NoV, 0.25) * G_SchlickGGX(NoL, 0.25);
}

/* sample */

vec3 sample_GGX(in vec2 xi, in float roughness) {
    float a = roughness * roughness;

    float phi = 2.0 * PI * xi.x;
    float cos_theta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    return vec3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
}

vec3 sample_clearcoat(in vec2 xi, in float roughness) {
    float a = mix(0.001, 0.1, roughness);

    float phi = 2.0 * PI * xi.x;
    float cos_theta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    return vec3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
}

vec3 sample_cosine_hemisphere() {
    float u1 = random();
    float u2 = random();

    float r = sqrt(u1);
    float phi = 2.0 * PI * u2;

    return vec3(r * cos(phi), r * sin(phi), sqrt(1.0 - u1));
}

vec3 sample_triangle(in vec3 p0, in vec3 p1, in vec3 p2) {
    float u = random();
    float v = random();

    float su = sqrt(u);

    float w0 = 1.0 - su;
    float w1 = su * (1.0 - v);
    float w2 = su * v;

    return w0 * p0 + w1 * p1 + w2 * p2;
}

uint sample_light(out float light_pdf) {
    float r = random() * data.total_light_area;

    float accum = 0.0;
    for (uint i = 0u; i < light_areas.length(); ++i) {
        accum += light_areas[i];
        if (r <= accum) {
            light_pdf = light_areas[i] / data.total_light_area;
            return i;
        }
    }

    light_pdf = light_areas[0] / data.total_light_area;
    return 0;
}

void sample_light_point(in uint base, out vec3 position, out vec3 normal, out vec3 emission) {
    uint i0 = indices[base + 0];
    uint i1 = indices[base + 1];
    uint i2 = indices[base + 2];

    vec3 p0 = vertices[i0].position;
    vec3 p1 = vertices[i1].position;
    vec3 p2 = vertices[i2].position;

    position = sample_triangle(p0, p1, p2);

    normal = normalize(cross(p1 - p0, p2 - p0));

    emission = materials[vertices[i0].material].emission;
}

/* pdf */

float pdf_specular(in vec3 N, in vec3 H, in vec3 V, in float roughness) {
    float NoH = max(dot(N, H), 0.0);
    float VoH = max(dot(V, H), 0.0);
    if (NoH <= 0.0 || VoH <= 0.0) {
        return 0.0;
    }

    float D = D_GGX(NoH, roughness);

    return (D * NoH) / max(4.0 * VoH, EPSILON);
}

float pdf_light(in float light_select_pdf, in float area, in vec3 Ln, in vec3 L, in float dist2) {
    if (area < EPSILON) {
        return 0.0;
    }

    float cos_light = dot(Ln, -L);
    if (cos_light < EPSILON) {
        return 0.0;
    }

    float pdf_area = light_select_pdf / area;
    return pdf_area * dist2 / cos_light;
}

float pdf_diffuse(in vec3 N, in vec3 L) {
    float NoL = max(dot(N, L), 0.0);
    return NoL / PI;
}

float pdf_clearcoat(in vec3 N, in vec3 H, in vec3 V, in float roughness) {
    float NoH = max(dot(N, H), 0.0);
    float VoH = max(dot(V, H), 0.0);
    if (NoH <= 0.0 || VoH <= 0.0) {
        return 0.0;
    }

    float D = D_GGX_Clearcoat(NoH, roughness);

    return (D * NoH) / max(4.0 * VoH, EPSILON);
}

float pdf_bsdf(in vec3 N, in vec3 H, in vec3 L, in uint material, in float w_diffuse, in float w_specular, in float w_clearcoat) {
    float NoL = max(dot(N, L), 0.0);
    if (NoL < 0.0) {
        return 0.0;
    }

    material_t mat = materials[material];

    float pdf = 0.0;

    if (w_diffuse > 0.0) {
        pdf += w_diffuse * pdf_diffuse(N, L);
    }

    if (w_specular > 0.0) {
        pdf += w_specular * pdf_specular(N, H, L, mat.roughness);
    }

    if (w_clearcoat > 0.0) {
        pdf += w_clearcoat * pdf_clearcoat(N, H, L, mat.clearcoat_roughness);
    }

    return pdf;
}

/* scatter */

bool visible(in vec3 Sp, in vec3 Sn, in vec3 Lp) {
    vec3 direction = Lp - Sp;
    float distance = length(direction);
    direction /= distance;

    ray_t shadow_ray;
    shadow_ray.origin = Sp + Sn * EPSILON;
    shadow_ray.direction = direction;

    record_t tmp;
    tmp.t = distance - 2.0 * EPSILON;

    return !hit_bvh(shadow_ray, true, tmp);
}

float power_heuristic(in float pdf_a, in float pdf_b) {
    float a2 = pdf_a * pdf_a;
    float b2 = pdf_b * pdf_b;
    float denom = a2 + b2;
    if (denom < EPSILON) {
        return 0.0;
    }
    return clamp(a2 / denom, 0.0, 1.0);
}

bool scatter(inout ray_t ray, in record_t rec, inout vec3 throughput, inout vec3 radiance) {
    if (rec.material >= materials.length()) {
        return false;
    }

    material_t mat = materials[rec.material];

    if (dot(mat.emission, mat.emission) > 0.0) {
        radiance += throughput * mat.emission;
        return false;
    }

    vec3 N = rec.normal;
    vec3 V = normalize(-ray.direction);
    float NoV = max(dot(N, V), 0.0);
    if (NoV <= 0.0) {
        return false;
    }

    vec3 F0 = mix(vec3(0.04), mat.albedo, mat.metallic);

    float w_diffuse = (1.0 - mat.metallic);
    float w_specular = mix(0.04, 1.0, mat.metallic);
    float w_clearcoat = mat.clearcoat_thickness * 0.25;

    float sum = w_diffuse + w_specular + w_clearcoat;
    w_diffuse /= sum;
    w_specular /= sum;
    w_clearcoat /= sum;

    {
        float light_select_pdf;
        uint index = sample_light(light_select_pdf);

        uint base = light_triangles[index];

        vec3 Lp, Ln, Le;
        sample_light_point(base, Lp, Ln, Le);

        vec3 L = Lp - rec.position;
        float dist2 = dot(L, L);
        float dist = sqrt(dist2);
        L /= dist;

        vec3 H = normalize(V + L);
        float bsdf_pdf = pdf_bsdf(N, H, L, rec.material, w_diffuse, w_specular, w_clearcoat);

        if (visible(rec.position, rec.normal, Lp)) {
            float NoL = max(dot(N, L), 0.0);
            if (NoL > EPSILON) {
                float area = light_areas[index];
                float light_pdf = pdf_light(light_select_pdf, area, Ln, L, dist2);

                if (light_pdf > EPSILON) {
                    vec3 F = fresnel_schlick(NoV, F0);
                    vec3 kd = (1.0 - F) * (1.0 - mat.metallic);

                    vec3 brdf = kd * mat.albedo / PI;

                    float light_pdf_c = max(light_pdf, EPSILON);
                    float bsdf_pdf_c = max(bsdf_pdf, EPSILON);

                    float w = power_heuristic(light_pdf_c, bsdf_pdf_c);

                    vec3 contrib = throughput * brdf * Le * NoL * w / light_pdf_c;
                    radiance += contrib;
                }
            }
        }
    }

    mat3 tbn = make_tbn(N);

    float r = random();

    vec3 L;
    if (r < w_specular) {
        vec2 xi = vec2(random(), random());
        vec3 H = normalize(tbn * sample_GGX(xi, max(0.001, mat.roughness)));
        L = reflect(-V, H);

        float NoL = max(dot(N, L), 0.0);
        if (NoL <= 0.0) {
            return false;
        }

        float NoH = max(dot(N, H), 0.0);
        float VoH = max(dot(V, H), 0.0);

        float D = D_GGX(NoH, mat.roughness);
        float G = G_Smith(NoV, NoL, mat.roughness);
        vec3 F = fresnel_schlick(VoH, F0);

        vec3 brdf = (D * G * F) / max(4.0 * NoV * NoL, 1e-5);
        float pdf = pdf_specular(N, H, V, mat.roughness);

        throughput *= brdf * NoL / (pdf * w_specular);
    }
    else if (r < w_specular + w_clearcoat) {
        vec2 xi = vec2(random(), random());
        vec3 H = normalize(tbn * sample_clearcoat(xi, mat.clearcoat_roughness));
        L = reflect(-V, H);

        float NoL = max(dot(N, L), 0.0);
        if (NoL <= 0.0) {
            return false;
        }

        float NoH = max(dot(N, H), 0.0);
        float VoH = max(dot(V, H), 0.0);

        float D = D_GGX_Clearcoat(NoH, mat.clearcoat_roughness);
        float G = G_Clearcoat(NoV, NoL);
        vec3 F = vec3(0.04);

        vec3 brdf = (D * G * F) / max(4.0 * NoV * NoL, 1e-5);
        float pdf = (D * NoH) / max(4.0 * VoH, 1e-5);

        throughput *= brdf * NoL / (pdf * w_clearcoat);
    }
    else {
        vec3 L_local = sample_cosine_hemisphere();
        L = normalize(tbn * L_local);

        float NoL = max(dot(N, L), 0.0);
        if (NoL <= 0.0) {
            return false;
        }

        vec3 F = fresnel_schlick(NoV, F0);
        vec3 kd = (1.0 - F) * (1.0 - mat.metallic);

        vec3 brdf = kd * mat.albedo / PI;
        float pdf  = NoL / PI;

        if (mat.sheen > 0.0) {
            float NoH = max(dot(N, normalize(V + L)), 0.0);
            float D = D_Charlie(NoH, mat.roughness);
            vec3 Fs = fresnel_sheen(NoV, mat.albedo);
            brdf += mat.sheen * D * Fs;
        }

        throughput *= brdf * NoL / (pdf * w_diffuse);
    }

    ray.origin = rec.position + N * EPSILON;
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
    ivec2 size = imageSize(accumulation);

    vec4 samples = imageLoad(accumulation, pixel);

    ivec2 tiles = (size + ivec2(TILE_W - 1, TILE_H - 1)) / ivec2(TILE_W, TILE_H);

    uint tile_count = tiles.x * tiles.y;
    uint tile_index = data.frame % tile_count;
    uint sample_index = data.frame / tile_count;
    ivec2 tile = ivec2(tile_index % tiles.x, tile_index / tiles.x);

    ivec2 tile_min = tile * ivec2(TILE_W, TILE_H);
    ivec2 tile_max = tile_min + ivec2(TILE_W, TILE_H);

    if (pixel.x < tile_min.x || pixel.y < tile_min.y || pixel.x >= tile_max.x || pixel.y >= tile_max.y) {
        color = vec4(sqrt(samples.rgb / (float(sample_index) + 1.0)), 1.0);
        return;
    }

    if (sample_index >= MAX_SAMPLES) {
        color = vec4(sqrt(samples.rgb / (float(MAX_SAMPLES) + 1.0)), 1.0);
        return;
    }

    vec2 grid_sample = (vec2(sample_index % MAX_SAMPLES_ROOT, sample_index / MAX_SAMPLES_ROOT) + vec2(random(), random())) * INV_MAX_SAMPLES_ROOT - 0.5;

    seed = uint(pixel.x) * 1973u ^ uint(pixel.y) * 9277u ^ sample_index * 26699u;

    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);

    vec2 pixel_delta = 1.0 / vec2(size);
    vec2 offset = vec2(pixel_delta.x * grid_sample.x, pixel_delta.y * grid_sample.y);

    vec2 ndc = (uv + offset) * 2.0 - 1.0;

    vec4 clip = vec4(ndc, -1.0, 1.0);
    vec4 view = data.inv_proj * clip;
    view /= view.w;

    vec3 direction = normalize((data.inv_view * vec4(view.xyz, 0.0)).xyz);

    ray_t ray;
    ray.origin = data.origin;
    ray.direction = direction;

    record_t rec;
    for (uint bounce = 0u; bounce < 5u; ++bounce) {

        rec.t = 1e30;

        if (!hit_bvh(ray, false, rec)) {
            radiance += throughput * miss(ray);
            break;
        }

        if (!scatter(ray, rec, throughput, radiance)) {
            break;
        }
    }

    samples = vec4(samples.rgb + radiance, 1.0);
    imageStore(accumulation, pixel, samples);

    color = vec4(sqrt(samples.rgb / (float(sample_index) + 1.0)), 1.0);
}
