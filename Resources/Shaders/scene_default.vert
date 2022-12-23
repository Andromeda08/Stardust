#version 460

layout (set = 0, binding = 0) uniform CameraUniformData {
    mat4 view;
    mat4 proj;
    mat4 view_inverse;
    mat4 proj_inverse;
    vec4 eye;
} ubo;

layout (push_constant) uniform PushConstants {
    vec4 time;
    mat4 model;
    vec4 color;
} pc;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec2 inUV;

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 fragColor;
layout (location = 2) out vec3 fragNormal;
layout (location = 3) out vec2 fragUV;

void main() {
    gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPosition, 1.0);
    fragPos     = (pc.model * vec4(inPosition, 0.0)).xyz;
    fragColor   = pc.color.xyz;
    fragNormal  = inNormal;
    fragUV      = inUV;
}