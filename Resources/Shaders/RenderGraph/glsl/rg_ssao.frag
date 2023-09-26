#version 460

layout (location = 0) in vec2 f_uv;

layout (set = 0, binding = 0) uniform CameraDataUniform {
    mat4 view;
    mat4 proj;
    mat4 view_inverse;
    mat4 proj_inverse;
    vec4 eye;
} camera;

layout (set = 0, binding = 1) uniform Options {
    ivec4 sampleCount;
    vec4 radius_bias; /* 0.5, 0.025 */
    vec4 samples[64]; /* Capped at 64 */
    vec4 noise[32]; /* Always at 32 */
} options;

layout (set = 0, binding = 2) uniform sampler2D u_position;
layout (set = 0, binding = 3) uniform sampler2D u_normal;

layout (location = 0) out float outColor;

void main() {
    vec2 uv = f_uv;
    uv.y = -f_uv.y;

    vec3 position = texture(u_position, uv).rgb;
    vec3 normal = normalize(texture(u_normal, uv).rgb);

    int sample_count = (options.sampleCount.x >= 64) ? 64 : options.sampleCount.x;
    float radius = options.radius_bias.x;
    float bias = options.radius_bias.y;

    int  noiseS = int(sqrt(32));
    int  noiseX = int(gl_FragCoord.x - 0.5) % noiseS;
    int  noiseY = int(gl_FragCoord.y - 0.5) % noiseS;
    vec3 random = options.noise[noiseX + (noiseY * noiseS)].xyz;

    vec3 tangent   = normalize(random - normal * dot(random, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float occlusion = sample_count;
    for (int i = 0; i < sample_count; i++)
    {
        vec3 sample_pos = TBN * options.samples[i].xyz;
        sample_pos = position + sample_pos * radius;

        vec4 offset = vec4(sample_pos, 1.0);
        offset = camera.proj * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float occluded = 0;
        if (sample_pos.y + bias <= offset.y) {
            occluded = 0;
        } else {
            occluded = 1;
        }

        float intensity = smoothstep(0, 1, radius / abs(position.y - offset.y));
        occluded  *= intensity;
        occlusion -= occluded;

        //float sample_depth = texture(u_position, offset.xy).z;
        //float range_check = smoothstep(0.0, 1.0, radius / abs(position.z - sample_depth));
        //occlusion += (sample_depth >= position.z + bias ? 1.0 : 0.0) * range_check;
    }

    occlusion /= sample_count;
    occlusion = pow(occlusion, 1.1);
    occlusion = 1.1 * (occlusion - 0.5) + 0.5;

    outColor = occlusion;
}