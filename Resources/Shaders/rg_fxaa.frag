#version 460

/*
 * Fast Approximate Anti-Aliasing shader
 * based on the FXAA whitepaper by Timothy Lottes (NVIDIA), Feburary 2009
 * https://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf
 * Further resources:
 * http://iryoku.com/aacourse/downloads/09-FXAA-3.11-in-15-Slides.pdf
 */

layout (location = 0) in vec2 f_uv;

layout (set = 0, binding = 0) uniform sampler2D uInputTexture;

layout (push_constant) uniform FXAAPushConstant {
    ivec4 params;
} pc;

layout (location = 0) out vec4 fragColor;

// FXAA Parameters
const float FXAA_EDGE_THRESHOLD = 1.0 / 8.0;
const float FXAA_EDGE_THRESHOLD_MIN = 1.0 / 16.0;
const float FXAA_SUBPIX = 1;
const float FXAA_SUBPIX_CAP = 3.0 / 4.0;
const float FXAA_SUBPIX_TRIM = 1.0 / 4.0;
const uint FXAA_SEARCH_STEPS = 8;
const uint FXAA_SEARCH_ACCELERATION = 1;

const float FXAA_SPAN_MAX = 8.0;
const float FXAA_REDUCE_MUL = 1.0 / 8.0;
const float FXAA_REDUCE_MIN = 1.0 / 128.0;

// https://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf
float fxaa_luma(vec3 rgb) {
    return rgb.y * (0.587 / 0.299) + rgb.x;
}

// alternatively, see: https://en.wikipedia.org/wiki/Grayscale
const vec3 to_luma = vec3(0.299, 0.587, 0.114);
float fxaa_luma_dot(vec3 rgb) {
    return dot(rgb, to_luma);
}

vec3 fxaa() {
    vec3 rgbM = textureLodOffset(uInputTexture, f_uv, 0.0, ivec2( 0,  0)).rgb;

    // Options from PushConstant
    int enable = pc.params[0];
    int show_edges = pc.params[1];

    if (enable == 0) {
        return rgbM;
    }

    vec3 rgbNW = textureLodOffset(uInputTexture, f_uv, 0.0, ivec2(-1,  1)).rgb;
    vec3 rgbNE = textureLodOffset(uInputTexture, f_uv, 0.0, ivec2( 1,  1)).rgb;
    vec3 rgbSW = textureLodOffset(uInputTexture, f_uv, 0.0, ivec2(-1, -1)).rgb;
    vec3 rgbSE = textureLodOffset(uInputTexture, f_uv, 0.0, ivec2( 1, -1)).rgb;

    float lumaM = fxaa_luma(rgbM);
    float lumaNW = fxaa_luma(rgbNW);
    float lumaNE = fxaa_luma(rgbNE);
    float lumaSW = fxaa_luma(rgbSW);
    float lumaSE = fxaa_luma(rgbSE);

    // Calculate luma range
    float rangeMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSE, lumaSW)));
    float rangeMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSE, lumaSW)));
    float range    = rangeMax - rangeMin;

    // If contrast is lower than a maximum threshold do no AA and return.
    if (range < max(FXAA_EDGE_THRESHOLD_MIN, rangeMax * FXAA_EDGE_THRESHOLD)) {
        return rgbM;
    }

    vec2 samplingDirection;
    samplingDirection.x = -(lumaNW + lumaNE) - (lumaSW + lumaSE);
    samplingDirection.y =  (lumaNW + lumaSW) - (lumaNE + lumaSE);

    float samplingDirectionReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.25 * FXAA_REDUCE_MUL, FXAA_REDUCE_MIN);
    float minSamplingDirectionFactor = 1.0 / (min(abs(samplingDirection.x), abs(samplingDirection.y)) + samplingDirectionReduce);

    vec2 size = textureSize(uInputTexture, 0);
    vec2 texel_step = vec2(1.0f / size.x, 1.0f / size.y);

    samplingDirection = clamp(samplingDirection * minSamplingDirectionFactor, vec2(-FXAA_SPAN_MAX), vec2(FXAA_SPAN_MAX)) * texel_step;

    vec3 rgbSampleNeg = texture(uInputTexture, f_uv + samplingDirection * (1.0 / 3.0 - 0.5)).rgb;
    vec3 rgbSamplePos = texture(uInputTexture, f_uv + samplingDirection * (2.0 / 3.0 - 0.5)).rgb;

    vec3 rgbTwoSamples = (rgbSamplePos + rgbSampleNeg) * 0.5;

    vec3 rgbSampleNegOuter = texture(uInputTexture, f_uv + samplingDirection * (0.0 / 3.0 - 0.5)).rgb;
    vec3 rgbSamplePosOuter = texture(uInputTexture, f_uv + samplingDirection * (3.0 / 3.0 - 0.5)).rgb;
    vec3 rgbFourSamples = (rgbSamplePosOuter + rgbSampleNegOuter) * 0.25 + rgbTwoSamples * 0.5;

    float rgbFourSamplesLuma = fxaa_luma(rgbFourSamples);

    // Are outer samples of the tab beyond the edge if yes use two samples, if no use four samples.
    if (rgbFourSamplesLuma < rangeMin || rgbFourSamplesLuma > rangeMax) {
        return rgbTwoSamples;
    } else {
        return rgbFourSamples;
    }
}

void main() {
    fragColor = vec4(fxaa(), 1.0);
}
