#version 450
layout(location = 0) in vec2 outUV;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D noisyTxt;
layout(set = 0, binding = 1) uniform sampler2D aoTxt;

layout(push_constant) uniform shaderInformation
{
    float aspectRatio;
}
pushc;

void main()
{
    vec2 uv = outUV;
    uv.y = -outUV.y;
    float gamma = 1.0 / 2.2;
    vec4  color = texture(noisyTxt, uv);
    float ao    = texture(aoTxt, uv).x;

    fragColor = vec4(pow(color.rgb * ao, vec3(gamma)), 1.0);
}