#version 330 core

in vec3 normal;
in vec3 fragPos;

uniform vec3 color;
uniform vec3 cameraPos;

out vec4 FragColor;

void main()
{
    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(cameraPos - fragPos);
    float fresnel = 1.0 - pow(max(1.0 - dot(viewDir, norm), 0.0), 4.0);
    vec3 color = mix(vec3(1.0, 0.2, 0.0), vec3(1.0, 0.8, 0.3), fresnel);
    FragColor = vec4(color * 3.0, 1.0);
}