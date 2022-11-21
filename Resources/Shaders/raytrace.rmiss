#version 460
#extension GL_EXT_ray_tracing : require

struct HitPayload
{
    vec3 hit_value;
};

layout(location = 0) rayPayloadInEXT HitPayload prd;

void main()
{
    vec3 clear_color = vec3(49, 50, 68) / 255;
    prd.hit_value = clear_color;
}