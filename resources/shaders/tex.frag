#version 460 core

#define MAX_POINT_LIGHTS 15
#define MAX_SPOT_LIGHTS 15

struct AmbientLight {
    vec3 color;
};

struct DirectionalLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 texcoord;
} fs_in;

out vec4 FragColor;

uniform sampler2D tex0;
uniform vec3 viewPos;

uniform AmbientLight ambientLight;
uniform DirectionalLight dirLights[1];
uniform int numPointLights;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int numSpotLights;
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];

vec3 calculatePhongLighting(vec3 lightDir, vec3 normal, vec3 viewDir, vec3 ambient, vec3 diffuse, vec3 specular, vec3 texColor) {
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    
    return ambient * texColor + diffuse * diff * texColor + specular * spec * texColor;
}

vec3 CalcDirLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 texColor) {
    vec3 lightDir = normalize(-light.direction);
    return calculatePhongLighting(lightDir, normal, viewDir, light.ambient, light.diffuse, light.specular, texColor);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor) {
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    
    vec3 lighting = calculatePhongLighting(lightDir, normal, viewDir, light.ambient, light.diffuse, light.specular, texColor);
    return lighting * attenuation;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor) {
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    vec3 lighting = calculatePhongLighting(lightDir, normal, viewDir, light.ambient, light.diffuse, light.specular, texColor);
    return lighting * attenuation * intensity;
}

void main() {
    vec3 norm = normalize(fs_in.Normal);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec4 texSample = texture(tex0, fs_in.texcoord);
    vec3 texColor = texSample.rgb;
    float alpha = texSample.a;

    vec3 result = ambientLight.color * texColor;

    result += CalcDirLight(dirLights[0], norm, viewDir, texColor);

    int i = 0;
    while (i < numPointLights) {
        result += CalcPointLight(pointLights[i], norm, fs_in.FragPos, viewDir, texColor);
        i++;
    }

    i = 0;
    while (i < numSpotLights) {
        result += CalcSpotLight(spotLights[i], norm, fs_in.FragPos, viewDir, texColor);
        i++;
    }

    FragColor = vec4(result, alpha);
}