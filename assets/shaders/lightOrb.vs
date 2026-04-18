#version 410 core

layout(location = 0) in vec3 in_position;

// uniforms
uniform mat4 model;
uniform mat4 view_proj;

// man they really got nothing in here
void main()
{
    gl_Position = view_proj * model * vec4(in_position, 1.0);
}