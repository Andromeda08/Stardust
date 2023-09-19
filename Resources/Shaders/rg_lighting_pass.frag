#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_query : enable
#extension GL_GOOGLE_include_directive : enable

layout (location = 0) in vec2 f_uv;

struct CameraUniformData {
    mat4 view;
    mat4 proj;
    mat4 view_inverse;
    mat4 proj_inverse;
    vec4 eye;
};

layout (set = 0, binding = 0) uniform LightingPassUniform {
    vec4 light_pos;
    CameraUniformData camera_data;
} ubo;

layout (set = 0, binding = 1) uniform sampler2D u_gbuffer;
layout (set = 0, binding = 2) uniform sampler2D u_albedo;
layout (set = 0, binding = 3) uniform sampler2D u_depth;
layout (set = 0, binding = 4) uniform accelerationStructureEXT u_tlas;

layout (push_constant) uniform LightingPassPushConstant {
    // [0]: Enabled RayQuery shadows
    ivec4 flags_a;
} pc;

layout (location = 0) out vec4 outColor;

#define C_Stack_Max 3.402823466e+38f
// linearly maps a short 32767-32768 to a float -1-+1 //!! opt.?
float ShortToFloatM11(const int v) {
    return (v >= 0) ? (uintBitsToFloat(0x3F800000u | (uint(v) << 8)) - 1.0f) :
    (uintBitsToFloat((0x80000000u | 0x3F800000u) | (uint(-v) << 8)) + 1.0f);
}
vec3 DecompressUnitVec(uint packed) {
    if(packed != ~0u)  // sanity check, not needed as isvalid_unit_vec is called earlier
    {
        int       x     = int(packed & 0xFFFFu) - 32767;
        int       y     = int(packed >> 16) - 32767;
        const int maskx = x >> 31;
        const int masky = y >> 31;
        const int tmp0  = 32767 + maskx + masky;
        const int ymask = y ^ masky;
        const int tmp1  = tmp0 - (x ^ maskx);
        const int z     = tmp1 - ymask;
        float     zf;
        if(z < 0)
        {
            x  = (tmp0 - ymask) ^ maskx;
            y  = tmp1 ^ masky;
            zf = uintBitsToFloat((0x80000000u | 0x3F800000u) | (uint(-z) << 8)) + 1.0f;
        }
        else
        {
            zf = uintBitsToFloat(0x3F800000u | (uint(z) << 8)) - 1.0f;
        }
        return normalize(vec3(ShortToFloatM11(x), ShortToFloatM11(y), zf));
    }
    else
    {
        return vec3(C_Stack_Max);
    }
}

vec3 compute_diffuse(vec3 color, vec3 light_dir, vec3 normal) {
    float dot_nl = max(dot(normal, light_dir), 0.0);
    vec3 c = color * dot_nl;
    c += 0.1 * color;  // Ambient
    return c;
}

vec3 compute_specular(vec3 color, vec3 view_dir, vec3 light_dir, vec3 normal) {
    const float k_pi = 3.14159265;
    const float k_shininess = 2.5;

    const float k_energy_conservation = (2.0 + k_shininess) / (2.0 * k_pi);
    vec3 V = normalize(-view_dir);
    vec3 R = reflect(-light_dir, normal);
    float specular = k_energy_conservation * pow(max(dot(V, R), 0.0), k_shininess);

    return vec3(0.25 * specular);
}

void main() {
    float compressed_normal = texture(u_gbuffer, f_uv).w;

    vec3 i_worldNormal = DecompressUnitVec(floatBitsToUint(compressed_normal));
    vec3 i_worldPos = texture(u_gbuffer, f_uv).rgb;
    vec3 i_color = texture(u_albedo, f_uv).rgb;
    vec3 i_viewDir = ubo.camera_data.eye.xyz - i_worldPos;

    vec3 N = normalize(i_worldNormal);

    vec3 l_dir = ubo.light_pos.xyz - i_worldPos;

    vec3 L = normalize(l_dir);
    float light_distance = length(l_dir);

    vec3 diffuse = compute_diffuse(i_color, L, N);
    vec3 specular = compute_specular(i_color, i_viewDir, L, N);

    outColor = vec4(diffuse + specular, 1);

    vec3 origin = i_worldPos;
    vec3 direction = L;
    float t_min = 0.01;
    float t_max = light_distance;

    if (pc.flags_a.x == 1) {
        rayQueryEXT ray_query;
        rayQueryInitializeEXT(ray_query, u_tlas, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT, 0xFF, origin, t_min, direction, t_max);

        float shadow = 0.1;
        while(rayQueryProceedEXT(ray_query)) {}
        if (rayQueryGetIntersectionTypeEXT(ray_query, true) != gl_RayQueryCommittedIntersectionNoneEXT)
        {
            outColor *= shadow;
        }
    }
}
