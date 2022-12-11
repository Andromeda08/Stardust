#version 460

layout (set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 view;
    mat4 proj;
    mat4 view_inverse;
    mat4 proj_inverse;
    mat4 model;
} ubo;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec2 inUV;

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 fragColor;
layout (location = 2) out vec3 fragNormal;
layout (location = 3) out vec2 fragUV;


void main() {
    vec3 position = vec3(inPosition.x, inPosition.y, inPosition.z);

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position, 1.0);
    fragPos     = position;
    fragColor   = inColor;
    fragNormal  = inNormal;
    fragUV      = inUV;
}