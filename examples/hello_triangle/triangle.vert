#version 450

layout(location = 0) out vec3 out_color;

void main()
{
    vec3 positions[3] = 
    {
        { 0.0, -0.5, 0.0 },
        { 0.5,  0.5, 0.0 },
        { 0.5,  0.5, 0.0 }
    };
    vec3 colors[3] = 
    { 
        { 1.0, 0.0, 0.0 }, 
        { 0.0, 1.0, 0.0 }, 
        { 0.0, 0.0, 1.0 } 
    };

    out_color = colors[gl_VertexIndex];
    gl_Position = vec4(positions[gl_VertexIndex], 1.0);
}
