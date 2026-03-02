#version 410

precision mediump float;

out vec4 FragColor;

// varyings
in vec2 vs_texcoord;

// uniforms
uniform sampler2D screen;

void main()
{
  vec3 color = texture(screen, vs_texcoord).rgb;

  float gray = ((color.r + color.g + color.b) / 3.0);
  FragColor = vec4(gray, gray, gray, 1.0);
}