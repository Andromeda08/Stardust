#version 450

layout (binding = 1) uniform sampler2D texSampler;

layout (location = 0) in vec3 fragPos;
layout (location = 1) in vec3 fragColor;
layout (location = 2) in vec3 fragNormal;
layout (location = 3) in vec2 fragUV;

layout (location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(texture(texSampler, fragUV).rgb, 1.0);
}