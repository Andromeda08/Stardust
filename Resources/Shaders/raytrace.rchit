#version 460

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require

struct HitPayload
{
    vec3 hit_value;
};

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

struct Vertex
{
  vec3 position;
  vec3 color;
  vec3 normal;
  vec2 uv;
};

layout (location = 0) rayPayloadInEXT HitPayload prd;

layout (buffer_reference, scalar) buffer Vertices { Vertex v[]; };
layout (buffer_reference, scalar) buffer Indices { ivec3 i[]; };
layout (set = 0, binding = 3) buffer ObjDesc { ObjectDesc i; } obj_desc;

hitAttributeEXT vec3 attribs;

void main()
{
  ObjectDesc obj      = obj_desc.i;
  Vertices   vertices = Vertices(obj.vertex_addr);
  Indices    indices  = Indices(obj.index_addr);

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
  li.light_intensity = 100.0;
  li.light_position  = vec3(10, 10, 10);

  vec3 light = vec3(0, 0, 0);
  float light_intensity = li.light_intensity;
  float light_distance  = 100000.0;

  // Point light
  vec3 light_dir = li.light_position - world_pos;
  light_distance = length(light_dir);
  light_intensity = li.light_intensity / (light_distance * light_distance);
  light = normalize(light_dir);

  vec3 color = vec3(0, 1, 1);
  vec3 ambient = 0.05 * color;
  vec3 diffuse = color * max(dot(light, normal), 0.0);
  
  vec3 view_dir = normalize(gl_WorldRayOriginEXT - world_pos);
  vec3 idk = normalize(light + view_dir);
  vec3 specular = vec3(0.3) * pow(max(dot(normal, idk), 0.0), 32.0);

  prd.hit_value = vec3(ambient + diffuse + specular);
}
