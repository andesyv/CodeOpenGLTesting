#version 330 core
// Custom #include (see shader.h)
// #include "src/shaders/quat.vert"
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;

out vec3 normal;

void main()
{
    // Multiply with normal matrix (transpose inverse without scale)
    normal = aNormal;
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}