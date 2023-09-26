#version 460

layout(location = 0) in vec2 f_uv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D image;

void main() {
    const vec2 uv = vec2(f_uv.x, -f_uv.y);
    outColor = texture(image, uv);
}