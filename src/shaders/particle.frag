#version 330 core

in vec3 normal;
in vec3 color;

out vec4 FragColor;

void main()
{
    FragColor = vec4(vec3(1.0, 0.0, 0.0), 1.0);
}