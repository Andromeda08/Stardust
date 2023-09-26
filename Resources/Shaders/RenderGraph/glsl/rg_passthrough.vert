#version 460

layout (location = 0) out vec2 f_uv;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    f_uv        = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(f_uv * 2.0f - 1.0f, 1.0f, 1.0f);
    f_uv.y = -f_uv.y;
}
