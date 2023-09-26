#version 460

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inWorldNormal;
layout (location = 2) in vec2 inUv;
layout (location = 3) in vec3 inColor;
layout (location = 4) in vec3 inViewDir;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

void main()
{
    vec3 N = normalize(inWorldNormal);

    outPosition = vec4(inWorldPos, 1);
    outNormal = vec4(N, 1);
    outAlbedo = vec4(inColor, 1);
}

