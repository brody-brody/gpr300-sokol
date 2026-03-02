#version 410 core

in vec2 vs_texcoord;
out vec4 FragColor;

uniform sampler2D screen;
uniform float strength;

void main()
{
    // snapping UVs to grid
    vec2 pixelated = floor(vs_texcoord / strength) * strength;

    FragColor = texture(screen, pixelated);
}