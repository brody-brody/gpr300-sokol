#version 410 core

in vec2 vs_texcoord;
out vec4 FragColor;

uniform sampler2D screen;

void main()
{
    // subtract each channel from 1 to invert colors
    FragColor = vec4(1.0 - texture(screen, vs_texcoord).rgb, 1.0);
}