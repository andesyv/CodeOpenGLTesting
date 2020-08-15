#version 330 core
out vec4 FragColor;
  
in vec2 uv;

uniform sampler2D tex;
uniform sampler2D bloom;
uniform float exposure = 3.0;

void main()
{             
    const float gamma = 2.2;
    vec3 hdrColor = texture(tex, uv).rgb;      
    vec3 bloomColor = texture(bloom, uv).rgb;
    hdrColor += bloomColor; // additive blending
    // tone mapping
    vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    // also gamma correct while we're at it       
    result = pow(result, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0);
}