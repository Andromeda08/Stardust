#version 460

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_GOOGLE_include_directive : enable

#include "rtcommon.glsl"

struct LightInformation
{
    vec3  light_position;
    float light_intensity;
};

struct ObjectDesc
{
    uint64_t vertex_addr;
    uint64_t index_addr;
};

hitAttributeEXT vec2 attribs;

layout (location = 0) rayPayloadInEXT HitPayload prd;

layout (buffer_reference, scalar) buffer Vertices { Vertex v[]; };
layout (buffer_reference, scalar) buffer Indices { ivec3 i[]; };

layout (set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout (set = 0, binding = 2) uniform UBO {
    mat4 viewProjection;
    mat4 viewInverse;
    mat4 projInverse;
} uni;
layout (set = 0, binding = 3) buffer ObjDesc { ObjectDesc object_description; };
layout (set = 0, binding = 4) uniform MaterialUniform { Material materials[3]; };

layout (push_constant) uniform RtPushConstants
{
    int material_index;
};

void main()
{
    Vertices   vertices = Vertices(object_description.vertex_addr);
    Indices    indices  = Indices(object_description.index_addr);

    ivec3 idx = indices.i[gl_PrimitiveID];

    Vertex v0 = vertices.v[idx.x];
    Vertex v1 = vertices.v[idx.y];
    Vertex v2 = vertices.v[idx.z];

    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    const vec3 pos = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
    const vec3 world_pos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));
    const vec3 normal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
    const vec3 world_normal = normalize(vec3(normal * gl_WorldToObjectEXT));

    LightInformation li;
    li.light_intensity = 35000.0;
    li.light_position  = vec3(-16, 64, -76);

    vec3  light           = vec3(0);
    float light_intensity = li.light_intensity;
    float light_distance  = 1000.0;

    // Point light
    vec3 light_dir  = li.light_position - world_pos;
    light_distance  = length(light_dir);
    light_intensity = li.light_intensity / (light_distance * light_distance);
    light           = normalize(light_dir);

    int  matidx   = material_index;
    vec3 diffuse  = compute_diffuse(materials[matidx], light, world_normal);
    vec3 specular = compute_specular(materials[matidx], gl_WorldRayDirectionEXT, light, world_normal);

    prd.ray_origin   = world_pos;
    prd.ray_dir      = reflect(gl_WorldRayDirectionEXT, world_normal);
    prd.attenuation *= materials[matidx].specular.xyz;
    prd.done         = 0;

    prd.hit_value    = vec3((diffuse + specular) * light_intensity);
}
