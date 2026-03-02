#version 410 core

in vec2 vs_texcoord;
out vec4 FragColor;

uniform sampler2D screen;

void main()
{
    vec3 color = texture(screen, vs_texcoord).rgb;

    // luminance weights from the intenet
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    FragColor = vec4(vec3(gray), 1.0);
}