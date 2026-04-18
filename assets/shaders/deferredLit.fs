#version 410 core

out vec4 FragColor;

in vec2 UV;

// g buffer samplers
uniform sampler2D gPositions;
uniform sampler2D gNormals;
uniform sampler2D gAlbedo;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

// point light
struct PointLight {
    vec3 position;
    float radius;
    vec4 color;
};

#define MAX_POINT_LIGHTS 64

uniform PointLight _PointLights[MAX_POINT_LIGHTS];
uniform Material material;
uniform vec3 camera_position;

// linear falloff helper
float attenuateLinear(float distance, float radius)
{
    return clamp((radius - distance) / radius, 0.0, 1.0);
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 frag_pos)
{
    vec3 diff = light.position - frag_pos;
    vec3 to_light = normalize(diff);
    vec3 view_dir = normalize(camera_position - frag_pos);
    vec3 halfway = normalize(to_light + view_dir);

    float NdotL = max(dot(normal, to_light), 0.0);
    float NdotH = max(dot(normal, halfway), 0.0);

    vec3 diffuse = NdotL * material.diffuse * light.color.rgb;
    vec3 specular = pow(NdotH, material.shininess) * material.specular * light.color.rgb;

    float d = length(diff);
    float attenuation = attenuateLinear(d, light.radius);

    return (diffuse + specular) * attenuation;
}

void main()
{
    // sampling surface properties for screen pixel from g buffer
    vec3 frag_pos = texture(gPositions, UV).rgb;
    vec3 normal = texture(gNormals, UV).rgb;
    vec3 albedo = texture(gAlbedo, UV).rgb;

    // ambient only applied onec
    vec3 total_light = material.ambient;

    // get all point lights
    for (int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        total_light += calcPointLight(_PointLights[i], normal, frag_pos);
    }

    FragColor = vec4(albedo * total_light, 1.0);
}