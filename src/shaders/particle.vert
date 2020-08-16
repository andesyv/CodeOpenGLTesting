#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uProj;
uniform mat4 uView;
// uniform mat4 uModel;
layout (std140, binding = 0) uniform Positions
{
    vec3 pos[$PCOUNT * $TLENGTH];
};

out vec3 normal;
out vec3 color;

void main()
{
    // Multiply with normal matrix (transpose inverse without scale)
    normal = aNormal;
    color = vec3(floor(gl_InstanceID / float($PCOUNT)) * 0.1);
    mat4 model = mat4(mat3(1.0));
    model[3].xyz = pos[gl_InstanceID];
    gl_Position = uProj * uView * model * vec4(aPos, 1.0);
}