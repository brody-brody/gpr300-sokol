#version 410 core

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

in VS_OUT {
    vec3 frag_pos;
    vec3 normal;
    vec2 texcoord;
    vec4 frag_pos_light_space;
} fs_in;

out vec4 FragColor;

// uniforms
uniform sampler2D texture0;
uniform sampler2D shadowMap;
uniform Material material;
uniform Light light;
uniform vec3 camera_position;

float shadowCalculation(vec4 frag_pos_light_space, vec3 normal, vec3 light_dir)
{
    // perspective divide
    vec3 proj_coords = frag_pos_light_space.xyz / frag_pos_light_space.w;
    proj_coords = proj_coords * 0.5 + 0.5;

    float current_depth = proj_coords.z;

    // slope scaled bias
    float bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005);

    // pcf part
    float shadow = 0.0;
    vec2 texel_size = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcf_depth = texture(shadowMap, proj_coords.xy + vec2(x, y) * texel_size).r;
            shadow += current_depth - bias > pcf_depth ? 1.0 : 0.0;
        }
    }

    shadow /= 9.0;

    // no shadow beyond farplane
    if (proj_coords.z > 1.0)
        shadow = 0.0;

    return shadow;
}

// original blinnphong stuff
vec3 blinnPhong(vec3 normal, vec3 frag_pos, vec3 light_pos, vec3 light_color, float shadow)
{
    vec3 view_dir = normalize(camera_position - frag_pos);
    vec3 light_dir = normalize(light_pos - frag_pos);
    vec3 halfway_dir = normalize(light_dir + view_dir);

    float NdotL = max(dot(normal, light_dir), 0.0);
    float NdotH = max(dot(normal, halfway_dir), 0.0);

    vec3 ambient = material.ambient * light_color;
    vec3 diffuse = NdotL * material.diffuse * light_color;
    vec3 specular = pow(NdotH, material.shininess) * material.specular * light_color;

    return ambient + (1.0 - shadow) * (diffuse + specular);
}

void main()
{
    vec3 normal = normalize(fs_in.normal);
    vec3 light_dir = normalize(light.position - fs_in.frag_pos);

    float shadow = shadowCalculation(fs_in.frag_pos_light_space, normal, light_dir);
    vec3 lighting = blinnPhong(normal, fs_in.frag_pos, light.position, light.color, shadow);

    vec4 tex_color = texture(texture0, fs_in.texcoord);
    FragColor = vec4(lighting, 1.0) * tex_color;
}