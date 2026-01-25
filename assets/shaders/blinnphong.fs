#version 300 es

precision mediump float;

struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  float shininess;
};

struct Light {
  vec3 color;
  vec3 position;
};

out vec4 FragColor;

// varyings
in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_texcoord;

// uniforms
uniform sampler2D texture0;
uniform Material material;
uniform Light light;
uniform vec3 camera_position;

vec3 blinnPhong(vec3 normal, vec3 frag_pos, vec3 light_pos, vec3 light_color) {
  // normalize inputs
  vec3 view_dir = normalize(camera_position - frag_pos);
  vec3 light_dir = normalize(light_pos - frag_pos);
  vec3 halfway_dir = normalize(light_dir + view_dir);

  // dot products
  float NdotL = max(dot(normal, light_dir), 0.0);
  float NdotH = max(dot(normal, halfway_dir), 0.0);

  // components
  vec3 diffuse = NdotL * material.diffuse;
  vec3 specular = pow(NdotH, material.shininess * 128.0) * material.specular;

  //vec3 lighting = diffuse * material.diffuse + specular * material.specular + material.ambient;

  return (diffuse + specular) * light_color;
}

void main()
{
  vec3 normal = normalize(vs_normal);
  vec3 object_color = (normal * 0.5 + 0.5);
  vec3 light_color = blinnPhong(normal, vs_position, light.position, light.color);

  vec4 final_color = vec4(object_color * light_color, 1.0) * texture(texture0, vs_texcoord);

  FragColor = final_color;
}