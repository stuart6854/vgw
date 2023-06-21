#version 450

layout(location = 0) in vec2 in_texCoord;

layout(location = 0) out vec4 out_fragColor;

void main()
{
    out_fragColor = vec4(in_texCoord, 0.0, 1.0);
}
