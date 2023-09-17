#version 460

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inWorldNormal;
layout (location = 2) in vec2 inUv;
layout (location = 3) in vec3 inColor;
layout (location = 4) in vec3 inViewDir;

layout (location = 0) out vec4 outGBuffer;
layout (location = 1) out vec4 outAlbedo;

#define C_Stack_Max 3.402823466e+38f
uint CompressUnitVec(vec3 nv) {
    // map to octahedron and then flatten to 2D
    if((nv.x < C_Stack_Max) && !isinf(nv.x))
    {
        const float d = 32767.0f / (abs(nv.x) + abs(nv.y) + abs(nv.z));
        int         x = int(roundEven(nv.x * d));
        int         y = int(roundEven(nv.y * d));
        if(nv.z < 0.0f)
        {
            const int maskx = x >> 31;
            const int masky = y >> 31;
            const int tmp   = 32767 + maskx + masky;
            const int tmpx  = x;
            x               = (tmp - (y ^ masky)) ^ maskx;
            y               = (tmp - (tmpx ^ maskx)) ^ masky;
        }
        uint packed = (uint(y + 32767) << 16) | uint(x + 32767);
        if(packed == ~0u)
        return ~0x1u;
        return packed;
    }
    else
    {
        return ~0u;
    }
}

void main()
{
    vec3 N = normalize(inWorldNormal);

    outGBuffer = vec4(i_worldPos, uintBitsToFloat(CompressUnitVec(N)));
    outAlbedo = vec4(inColor, 1);
}

