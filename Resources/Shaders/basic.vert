#version 450

layout (set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 view_projection;
    mat4 model;
} ubo;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec2 fragUV;

void main() {
    gl_Position = ubo.view_projection * ubo.model * vec4(inPosition.x, -inPosition.y, inPosition.z, 1.0);
    fragColor = inColor;
    fragUV = inUV;
}