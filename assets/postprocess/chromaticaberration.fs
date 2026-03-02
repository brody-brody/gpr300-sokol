#version 410 core

in vec2 vs_texcoord;
out vec4 FragColor;

uniform sampler2D screen;
uniform float strength;

void main()
{
    // offset each channel slightly from center to fake lens split
    vec2 offset = (vs_texcoord - 0.5) * strength;

    float r = texture(screen, vs_texcoord - offset).r;
    float g = texture(screen, vs_texcoord).g;
    float b = texture(screen, vs_texcoord + offset).b;

    FragColor = vec4(r, g, b, 1.0);
}