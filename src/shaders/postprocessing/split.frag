#version 330 core

in vec2 uv;

uniform sampler2D tex;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 HDR;

void main()
{
    FragColor = texture(tex, uv);
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        HDR = vec4(FragColor.rgb, 1.0);
    else
        HDR = vec4(vec3(0.0), 1.0);
}