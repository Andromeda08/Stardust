#version 460

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#include "include/common.glsl"

struct ObjectDescription
{
    uint64_t vertex_address;
    uint64_t index_address;
};

hitAttributeEXT vec2 attributes;

layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; };
layout(buffer_reference, scalar) buffer Indices  { ivec3  i[]; };

layout(binding = 0, set = 0) uniform accelerationStructureEXT tlas;
layout(binding = 3, set = 0) buffer ObjDesc { ObjectDescription obj_desc; };

layout(location = 0) rayPayloadInEXT RTPayload payload;
layout(location = 1) rayPayloadEXT bool is_shadowed;

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void traceRay(RayDescription ray)
{
    traceRayEXT(tlas, ray.ray_flags, 0xff, 0, 0, 1, ray.origin.xyz, ray.t_min, ray.direction.xyz, ray.t_max, 1);
}

void main()
{
    Vertices vertices = Vertices(obj_desc.vertex_address);
    Indices  indices  = Indices(obj_desc.index_address);

    ivec3 idx = indices.i[gl_PrimitiveID];

    Vertex v0 = vertices.v[idx.x];
    Vertex v1 = vertices.v[idx.y];
    Vertex v2 = vertices.v[idx.z];

    const vec3 barycentrics = vec3(1.0 - attributes.x - attributes.y, attributes.x, attributes.y);

    const vec3 pos = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
    const vec3 world_pos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));

    const vec3 normal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
    const vec3 world_normal = normalize(vec3(normal * gl_WorldToObjectEXT));

    vec3 N = world_normal;
    vec3 l_dir = vec3(-12, 10, 5) - world_pos;

    vec3 L = normalize(l_dir);
    float light_distance = length(l_dir);

    vec2 id = vec2(gl_InstanceID, gl_InstanceID);
    vec3 color = vec3(rand(id) * sin(id.x) + 1.0, rand(id) * cos(id.y) + 1.0, rand(id));
    vec3 diffuse  = compute_diffuse(color, L, world_normal);
    vec3 specular = vec3(0);

    float attenuation = 1.0;

    if(dot(N, L) > 0)
    {
        RayDescription ray_desc;
        ray_desc.origin    = vec4(gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT, 0.0);
        ray_desc.direction = vec4(L, 0.0);
        ray_desc.t_min     = 0.001;
        ray_desc.t_max     = light_distance;
        ray_desc.ray_flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;

        is_shadowed = true;

        traceRay(ray_desc);
    }

    if(is_shadowed)
    {
        attenuation = 0.3;
    }
    else
    {
        specular = compute_specular(color, gl_WorldRayDirectionEXT, L, world_normal);
    }

    payload.ray_origin    = world_pos;
    payload.ray_direction = reflect(gl_WorldRayDirectionEXT, world_normal);
    payload.attenuation  *= vec3(0.45);
    payload.done          = 0;

    payload.hit_value = vec3(diffuse + specular) * attenuation;
}