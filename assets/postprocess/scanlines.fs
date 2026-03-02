#version 410 core

in vec2 vs_texcoord;
out vec4 FragColor;

uniform sampler2D screen;
uniform float strength;
uniform float resolution;

void main()
{
    vec3 color = texture(screen, vs_texcoord).rgb;

    // darken every other row based on resolution
    float line = mod(floor(vs_texcoord.y * resolution), 2.0);

    FragColor = vec4(color * (1.0 - line * strength), 1.0);
}