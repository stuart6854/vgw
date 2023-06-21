#version 450

layout(location = 0) in vec2 in_texCoord;

layout(location = 0) out vec4 out_fragColor;

layout(set = 0, binding = 1) uniform  sampler2D u_texture;

void main()
{
    vec3 diffuse = texture(u_texture, in_texCoord).rgb;
    out_fragColor = vec4(diffuse, 1.0);
}
