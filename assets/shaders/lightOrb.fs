#version 410 core

out vec4 FragColor;

uniform vec3 orb_color;

// man they really got nothing in here
void main()
{
    FragColor = vec4(orb_color, 1.0);
}