#version 410 core

in vec2 vs_texcoord;
out vec4 FragColor;

uniform sampler2D screen;
uniform float strength;

void main()
{
    vec2 texel = strength / textureSize(screen, 0);

    // sample cross pattern neighbors and subtract center to find edges
    vec3 top    = texture(screen, vs_texcoord + vec2( 0,  texel.y)).rgb;
    vec3 bottom = texture(screen, vs_texcoord + vec2( 0, -texel.y)).rgb;
    vec3 left   = texture(screen, vs_texcoord + vec2(-texel.x,  0)).rgb;
    vec3 right  = texture(screen, vs_texcoord + vec2( texel.x,  0)).rgb;
    
    vec3 center = texture(screen, vs_texcoord).rgb;

    vec3 edge = abs(top + bottom + left + right - center * 4.0);

    FragColor = vec4(edge, 1.0);
}