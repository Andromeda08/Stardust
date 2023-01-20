#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (push_constant) uniform PushConstantBlock {
    vec4 color;
    mat4 view;
    mat4 proj;
} pcb;

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec2 fragUV;
layout (location = 3) out vec3 fragColor;

void main() {
    gl_Position = pcb.proj * pcb.view * vec4(inPosition, 1.0);

    fragPos = (pcb.proj * pcb.view * vec4(inPosition, 1.0)).xyz;
    fragNormal = inNormal;
    fragUV = inUV;
    fragColor = pcb.color.xyz;
}