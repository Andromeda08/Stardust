// Blinn-Phong compute functions
vec3 compute_diffuse(vec3 color, vec3 light_dir, vec3 normal)
{
    float dot_nl = max(dot(normal, light_dir), 0.0);
    vec3 c = color * dot_nl;
    c += 0.1 * color;  // Ambient
    return c;
}

vec3 compute_specular(vec3 color, vec3 view_dir, vec3 light_dir, vec3 normal)
{
    const float k_pi = 3.14159265;
    const float k_shininess = 4.0;

    const float k_energy_conservation = (2.0 + k_shininess) / (2.0 * k_pi);
    vec3 V = normalize(-view_dir);
    vec3 R = reflect(-light_dir, normal);
    float specular = k_energy_conservation * pow(max(dot(V, R), 0.0), k_shininess);

    return vec3(0.3 * specular);
}


// Compress & decrompress normal(x, y, z) into a uint
// Source: https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR/blob/master/ray_tracing_ao/shaders/raycommon.glsl

#define C_STACK_MAX 3.402823466e+38f

// linearly maps a short 32767-32768 to a float -1-+1
float ShortToFloatM11(const int v)
{
    return (v >= 0)
        ? (uintBitsToFloat(0x3F800000u | (uint(v) << 8)) - 1.0f)
        : (uintBitsToFloat((0x80000000u | 0x3F800000u) | (uint(-v) << 8)) + 1.0f);
}

uint compress_uint_vec(vec3 v)
{
    // Map to octahedron and then flatten to 2D
    if ((v.x < C_STACK_MAX) && !isinf(v.x))
    {
        const float d = 32767.0 / (abs(v.x) + abs(v.y) + abs(v.z));
        int x = int(roundEven(v.x * d));
        int y = int(roundEven(v.y * d));
        if (v.z < 0.0)
        {
            const int mask_x = x >> 31;
            const int mask_y = y >> 31;
            const int tmp = 32767 + mask_x + mask_y;
            const int tmp_x = x;
            x = (tmp - (y ^ mask_y)) ^ mask_x;
            y = (tmp - (x ^ mask_x)) ^ mask_y;
        }
        uint packed = (uint(y + 32767) << 16) | uint(x + 32767);
        if (packed == ~0u)
        {
            return ~0x1u;
        }
        return packed;
    }
    else
    {
        return ~0u;
    }
}

vec3 decompress_uint_vec(uint packed)
{
    if(packed != ~0u)
    {
        int x = int(packed & 0xFFFFu) - 32767;
        int y = int(packed >> 16) - 32767;

        const int maskx = x >> 31;
        const int masky = y >> 31;
        const int tmp0  = 32767 + maskx + masky;
        const int ymask = y ^ masky;
        const int tmp1  = tmp0 - (x ^ maskx);
        const int z     = tmp1 - ymask;

        float zf;
        if(z < 0)
        {
            x  = (tmp0 - ymask) ^ maskx;
            y  = tmp1 ^ masky;
            zf = uintBitsToFloat((0x80000000u | 0x3F800000u) | (uint(-z) << 8)) + 1.0f;
        }
        else
        {
            zf = uintBitsToFloat(0x3F800000u | (uint(z) << 8)) - 1.0f;
        }
        return normalize(vec3(ShortToFloatM11(x), ShortToFloatM11(y), zf));
    }
    else
    {
        return vec3(C_STACK_MAX);
    }
}

#define EPS 0.05
const float M_PI = 3.141592653589;

void compute_default_basis(const vec3 normal, out vec3 x, out vec3 y)
{
    // ZAP's default coordinate system for compatibility
    vec3        z  = normal;
    const float yz = -z.y * z.z;
    y = normalize(((abs(z.z) > 0.99999f) ? vec3(-z.x * z.y, 1.0f - z.y * z.y, yz) : vec3(-z.x * z.z, yz, 1.0f - z.z * z.z)));

    x = cross(y, z);
}

vec3 offset_ray(in vec3 p, in vec3 n)
{
    const float intScale   = 256.0f;
    const float floatScale = 1.0f / 65536.0f;
    const float origin     = 1.0f / 32.0f;

    ivec3 of_i = ivec3(intScale * n.x, intScale * n.y, intScale * n.z);

    vec3 p_i = vec3(intBitsToFloat(floatBitsToInt(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
    intBitsToFloat(floatBitsToInt(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
    intBitsToFloat(floatBitsToInt(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));

    return vec3(abs(p.x) < origin ? p.x + floatScale * n.x : p_i.x,  //
    abs(p.y) < origin ? p.y + floatScale * n.y : p_i.y,  //
    abs(p.z) < origin ? p.z + floatScale * n.z : p_i.z);
}


// Generate a random unsigned int from two unsigned int values, using 16 pairs
// of rounds of the Tiny Encryption Algorithm. See Zafar, Olano, and Curtis,
// "GPU Random Numbers via the Tiny Encryption Algorithm"
uint tea(uint val0, uint val1)
{
    uint v0 = val0;
    uint v1 = val1;
    uint s0 = 0;

    for(uint n = 0; n < 16; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }

    return v0;
}

uvec2 pcg2d(uvec2 v)
{
    v = v * 1664525u + 1013904223u;

    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;

    v = v ^ (v >> 16u);

    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;

    v = v ^ (v >> 16u);

    return v;
}

// Generate a random unsigned int in [0, 2^24) given the previous RNG state
// using the Numerical Recipes linear congruential generator
uint lcg(inout uint prev)
{
    uint LCG_A = 1664525u;
    uint LCG_C = 1013904223u;
    prev       = (LCG_A * prev + LCG_C);
    return prev & 0x00FFFFFF;
}

// Generate a random float in [0, 1) given the previous RNG state
float rnd(inout uint seed)
{
    return (float(lcg(seed)) / float(0x01000000));
}