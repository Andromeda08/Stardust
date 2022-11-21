#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "rtcommon.glsl"

layout (location = 0) rayPayloadInEXT HitPayload prd;

void main()
{
    vec3 clear_color = vec3(49, 50, 68) / 255;
    prd.hit_value = clear_color;
}