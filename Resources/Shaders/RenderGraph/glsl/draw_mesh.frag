#version 460

layout(location = 0) in MeshInput {
    vec4 color;
    vec4 world_position;
    vec4 world_normal;
} m_input;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = m_input.color;
}
