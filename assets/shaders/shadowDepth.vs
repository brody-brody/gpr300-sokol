#version 410 core

layout(location = 0) in vec3 in_position;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main()
{
    gl_Position = lightSpaceMatrix * model * vec4(in_position, 1.0);
}