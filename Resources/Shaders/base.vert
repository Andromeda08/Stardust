#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (push_constant) uniform ObjectPushConstantData {
    mat4 model;
    vec4 color;
} obj_pcd;

layout (set = 0, binding = 0) uniform CameraUniformData
{
    mat4 view;
    mat4 proj;
    mat4 view_inverse;
    mat4 proj_inverse;
    vec4 eye;
} camera;


layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec2 fragUV;
layout (location = 3) out vec3 fragColor;

void main() {
    gl_Position = camera.proj * camera.view * obj_pcd.model * vec4(inPosition, 1.0);

    fragPos = (camera.proj * camera.view * obj_pcd.model * vec4(inPosition, 1.0)).xyz;
    fragNormal = inNormal;
    fragUV = inUV;
    fragColor = obj_pcd.color.xyz;
}