#version 330 core
in vec3 normal;
in vec3 fragPos;

uniform vec3 lightPos;
// uniform vec3 lightColor = vec3(1, 1, 1);
uniform vec3 color;
uniform vec3 cameraPos;

out vec4 FragColor;

void main()
{
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);

    // Ambient light
    float ambient = 0.1;

    // Diffuse light
    float diff = max(dot(norm, lightDir), 0.0);

    // Specular light
    vec3 viewDir = normalize(cameraPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32) * 0.5;

    FragColor = vec4((ambient + diff + spec) * color, 1.0);
}