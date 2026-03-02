#version 410 core

in vec2 vs_texcoord;
out vec4 FragColor;

uniform sampler2D screen;
uniform float strength;

void main()
{
    vec3 color = texture(screen, vs_texcoord).rgb;

    // remap channels around 0.5 midpoint to boost or reduce contrast
    FragColor = vec4(clamp((color - 0.5) * strength + 0.5, 0.0, 1.0), 1.0);
}