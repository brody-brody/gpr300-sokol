#version 300 es

precision mediump float;

out vec4 FragColor;

// varyings
in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_texcoord;

// uniforms
uniform sampler2D texture0;
uniform vec3 camera_position;

uniform float time;
uniform vec3 water_color;

void main()
{
  // offset texcoord in some direction by time passed in
  vec2 dir = vec2(1.0, 0.0);
  vec2 uv = vs_texcoord + vec2((time) * dir);

  uv.x += 0.01 * sin(uv.x * 3.5 + time);
  uv.y += 0.35 * sin(uv.y * 1.5 + time);

  vec4 sample1 = texture(texture0, uv * 1.0);
  vec4 sample2 = texture(texture0, uv * 1.2);

  vec3 object_color = vec3((sample1 * 0.75) - (sample2 * 0.25));

  FragColor = vec4(object_color + water_color, 1.0);
}