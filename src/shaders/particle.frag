#version 330 core

in vec3 normal;
in vec3 iColor;
in vec4 fragPos;

out vec4 FragColor;

void main()
{
    FragColor = vec4(iColor, 0.1);
}