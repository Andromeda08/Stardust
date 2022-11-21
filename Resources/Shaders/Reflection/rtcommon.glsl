struct HitPayload
{
    vec3 hit_value;
    int  depth;
    vec3 attenuation;
    int  done;
    vec3 ray_origin;
    vec3 ray_dir;
};

struct Vertex
{
    vec3 position;
    vec3 color;
    vec3 normal;
    vec2 uv;
};

struct Material
{
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float shininess;
};

vec3 compute_diffuse(Material mat, vec3 light_dir, vec3 normal)
{
    float dot_nl = max(dot(normal, light_dir), 0.0);
    vec3 c = mat.diffuse * dot_nl;
    c += mat.ambient;
    return c;
}

vec3 compute_specular(Material mat, vec3 view_dir, vec3 light_dir, vec3 normal)
{
    const float k_pi = 3.14159265;
    const float k_shininess = max(mat.shininess, 4.0);
    const float k_energy_conservation = (2.0 + k_shininess) / (2.0 * k_pi);
    vec3 v = normalize(-view_dir);
    vec3 r = reflect(-light_dir, normal);
    float specular = k_energy_conservation * pow(max(dot(v, r), 0.0), k_shininess);
    return vec3(mat.specular * specular);
}