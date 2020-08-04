#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;
// uniform ivec2 screenSize;

out vec3 normal;

void main()
{
    // float aspectRatio = float(screenSize.x) / screenSize.y;
    // Multiply with normal matrix (transpose inverse without scale)
    normal = aNormal;
    mat4 viewModel = uView * uModel;
    // viewModel[3].xyz = vec3(-aspectRatio, 0.0, -10.0);
    viewModel[3].xyz = vec3(0.0, 0.0, -10.0);
    mat4 UIMat = mat4(mat3(0.5));
    UIMat[3].xy = vec2(-0.9, -0.9);
    gl_Position = UIMat * uProj * viewModel * vec4(aPos, 1.0);
    // gl_Position.xy = vec2(-0.5, -0.5);
}