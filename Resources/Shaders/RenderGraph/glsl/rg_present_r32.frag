#version 460

layout(location = 0) in vec2 f_uv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D image;
layout(push_constant) uniform PresentPushConstant { ivec4 options; };

void main() {
    vec2 uv = vec2(f_uv.x, -f_uv.y);

    if (options.x == 1)
    {
        uv.y = f_uv.y;
    }

    vec4 color = texture(image, uv);
    outColor = vec4(color.r, color.r, color.r, 1.0);
}