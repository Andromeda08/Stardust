#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_query : enable

layout (location = 0) in vec3 i_worldPos;
layout (location = 1) in vec3 i_worldNormal;
layout (location = 2) in vec2 i_uv;
layout (location = 3) in vec3 i_color;
layout (location = 4) in vec3 i_viewDir;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform accelerationStructureEXT tlas;

vec3 g_light = vec3(5, 10, -10);

vec3 compute_diffuse(vec3 color, vec3 light_dir, vec3 normal)
{
    float dot_nl = max(dot(normal, light_dir), 0.0);
    vec3 c = color * dot_nl;
    c += 0.05 * color;  // Ambient
    return c;
}

vec3 compute_specular(vec3 color, vec3 view_dir, vec3 light_dir, vec3 normal)
{
    const float k_pi = 3.14159265;
    const float k_shininess = 4.0;

    const float k_energy_conservation = (2.0 + k_shininess) / (2.0 * k_pi);
    vec3 V = normalize(-view_dir);
    vec3 R = reflect(-light_dir, normal);
    float specular = k_energy_conservation * pow(max(dot(V, R), 0.0), k_shininess);

    return vec3(0.3 * specular);
}

void main()
{
    vec3 N = normalize(i_worldNormal);

    vec3 l_dir = g_light - i_worldPos;

    vec3 L = normalize(l_dir);
    float light_distance = length(l_dir);

    vec3 diffuse = compute_diffuse(i_color, L, N);
    vec3 specular = compute_specular(i_color, i_viewDir, L, N);

    outColor = vec4(diffuse + specular, 1);

    vec3 origin = i_worldPos;
    vec3 direction = L;
    float t_min = 0.01;
    float t_max = light_distance;

    rayQueryEXT ray_query;
    rayQueryInitializeEXT(ray_query, tlas, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, origin, t_min, direction, t_max);

    while(rayQueryProceedEXT(ray_query)) {}
    if (rayQueryGetIntersectionTypeEXT(ray_query, true) != gl_RayQueryCommittedIntersectionNoneEXT)
    {
        outColor *= 0.1;
    }
}
