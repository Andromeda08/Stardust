#version 460

layout (set = 0, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
    mat4 view_inverse;
    mat4 proj_inverse;
    vec4 camera_pos;
} ubo;


layout (location = 0) in vec3 fragPos;
layout (location = 1) in vec3 fragColor;
layout (location = 2) in vec3 fragNormal;
layout (location = 3) in vec2 fragUV;

layout (location = 0) out vec4 outColor;

void main() {
    vec3 light_pos = vec3(0, 100, 0);

    vec3 ambient = 0.05 * fragColor;

    vec3  light_dir = normalize(light_pos - fragPos);
    vec3  normal    = normalize(fragNormal);
    float diff      = max(dot(light_dir, normal), 0.0);
    vec3  diffuse   = diff * fragColor;

    vec3  view_dir  = normalize(ubo.camera_pos.xyz - fragPos);
    vec3  d_halfway = normalize(light_dir + view_dir);
    float spec      = pow(max(dot(normal, d_halfway), 0.0), 32.0);
    vec3  specular  = vec3(0.3) * spec;

    outColor = vec4(ambient + diffuse + specular, 1);
}