#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;

out vec3 normal;
out vec3 fragPos;

void main()
{
    // Multiply with normal matrix (transpose inverse without scale)
    normal = mat3(transpose(inverse(uModel))) * aNormal;

    fragPos = (uModel * vec4(aPos, 1.0)).xyz;
    gl_Position = uProj * uView * vec4(fragPos, 1.0);
}