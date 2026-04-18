#version 410 core

// mrt outputs - each maps to a color attachment on the g-buffer
layout(location = 0) out vec3 gPosition; // worldspace position
layout(location = 1) out vec3 gNormal;   // worldspace normal
layout(location = 2) out vec3 gAlbedo;   // base texture color before lighting

in VS_OUT {
    vec3 frag_pos;
    vec3 normal;
    vec2 texcoord;
} fs_in;

uniform sampler2D texture0;

void main()
{
    gPosition = fs_in.frag_pos;
    gNormal   = normalize(fs_in.normal);
    gAlbedo   = texture(texture0, fs_in.texcoord).rgb;
}