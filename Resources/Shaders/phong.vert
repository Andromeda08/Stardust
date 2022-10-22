#version 450

layout (set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 view_projection;
    mat4 model;
} ubo;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec2 inUV;

layout (location = 4) in vec3 instanceTranslate;
layout (location = 5) in vec3 instanceColor;
layout (location = 6) in vec3 instanceScale;

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 fragColor;
layout (location = 2) out vec3 fragNormal;
layout (location = 3) out vec2 fragUV;

mat4 translate_matrix(vec3 delta)
{
    return mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(delta, 1.0));
}

mat4 scale_matrix(vec3 delta)
{
    return mat4(
        vec4(delta.x, 0.0, 0.0, 0.0),
        vec4(0.0, delta.y, 0.0, 0.0),
        vec4(0.0, 0.0, delta.z, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0));
}


void main() {
    vec3 pos = vec3(inPosition.x, -inPosition.y, inPosition.z);

    gl_Position = ubo.view_projection * translate_matrix(instanceTranslate) * scale_matrix(instanceScale) * ubo.model * vec4(pos, 1.0);

    fragPos = (ubo.model * vec4(inPosition.x, -inPosition.y, inPosition.z, 1.0)).xyz;
    fragColor = instanceColor;
    fragNormal = inNormal;
    fragUV = inUV;
}