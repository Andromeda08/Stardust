#version 460

layout(location = 0) in MeshInput {
    vec4 meshlet_color;
    vec4 color;
    vec3 world_position;
    vec3 world_normal;
    vec2 uv;
    vec3 view_dir;
    vec4 current_position;
    vec4 previous_position;
    int  use_meshlet_color;
} m_input;

layout (location = 0) out vec4 out_position;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_albedo;
layout (location = 3) out vec4 out_motion_vector;

void main()
{
    vec3 N = normalize(m_input.world_normal);

    out_position = vec4(m_input.world_position, 1);
    out_normal = vec4(N, 1);

    if (m_input.use_meshlet_color != 0)
    {
        out_albedo = vec4(m_input.meshlet_color.xyz, 1);
    }
    else
    {
        out_albedo = vec4(m_input.color.xyz, 1);
    }

    vec3 a = (m_input.current_position / m_input.current_position.w).xyz;
    vec3 b = (m_input.previous_position / m_input.previous_position.w).xyz;
    vec2 mvec = (a - b).xy * 0.5;

    out_motion_vector = vec4(mvec, 0.0, 1.0);
}
