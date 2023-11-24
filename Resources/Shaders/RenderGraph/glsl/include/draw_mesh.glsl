struct CameraData
{
    mat4 view;
    mat4 proj;
    mat4 view_inverse;
    mat4 proj_inverse;
    vec4 eye;
};

struct Vertex
{
    vec3 position;
    vec3 normal;
    vec2 uv;
};

struct Meshlet
{
//    ivec3 indices;
    uint vertices[42];
    uint indices[126];
    uint vertex_count;
    uint index_count;
};

struct MeshShaderOutput
{
    vec4 color;
};

const int MESHLET_COLOR_COUNT = 10;
const vec3 meshlet_colors[MESHLET_COLOR_COUNT] = {
    vec3(1,0,0), vec3(0,1,0), vec3(0,0,1), vec3(1,1,0), vec3(1,0,1),
    vec3(0,1,1), vec3(1,0.5,0), vec3(0.5,1,0), vec3(0,0.5,1), vec3(1,1,1)
};