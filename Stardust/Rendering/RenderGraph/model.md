### Shading push constant
Basic color shading model:
```glsl
struct Material
{
    mat4 model_matrix;
    vec4 color;
};
```
PBR Shading model:
```glsl
struct Material
{
    image2D albedo;
    image2D normal;
    image2D roughness;
    image2D ambient_occlusion;
};
```