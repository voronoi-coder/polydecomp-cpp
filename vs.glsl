#version 410

layout (location = 0) in vec2 position;

uniform mat4 matrix;
uniform vec4 u_color;

out vec4 vs_color;

void main()
{
    gl_Position = matrix * vec4(position, 0.0, 1.0);
    vs_color = u_color;
}


