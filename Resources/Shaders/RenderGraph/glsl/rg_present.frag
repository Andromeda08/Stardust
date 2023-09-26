#version 460

layout(location = 0) in vec2 f_uv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D image;

void main() {
    outColor = texture(image, f_uv);
}