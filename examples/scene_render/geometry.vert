#version 450

layout(location = 0) in vec3 attr_position;
layout(location = 1) in vec3 attr_normal;
layout(location = 2) in vec2 attr_texCoord;

layout(location = 0) out vec2 out_texCoord;

layout(set = 0, binding = 0) uniform UniformData
{
    mat4 projMatrix;
    mat4 viewMatrix;
} uniforms;

layout (push_constant) uniform Constants
{
    mat4 worldMatrix;
} constants;

void main()
{
    vec4 worldPosition = constants.worldMatrix * vec4(attr_position, 1.0);
    gl_Position = uniforms.projMatrix * uniforms.viewMatrix * worldPosition;
    out_texCoord = attr_texCoord;
}
