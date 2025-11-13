#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

struct PointLight {
    vec3 position;
    vec3 color;
};

const int MAX_LIGHTS = 4;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
    int numLights;
    PointLight lights[MAX_LIGHTS];
} ubo;

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(ubo.cameraPos - fragWorldPos);

    // Ambient lighting (global fill)
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);

    // --- Hardcoded sunlight (directional light) ---
    vec3 sunDir = normalize(vec3(-0.5, -1.0, -0.3)); // direction *from* which sunlight comes
    vec3 sunColor = vec3(1.0, 0.95, 0.8);            // slightly warm tone

    float diffSun = max(dot(normal, -sunDir), 0.0); // note the negation; light shines *toward* the surface
    vec3 diffuseSun = diffSun * sunColor;

    float specularStrength = 0.5;
    vec3 halfwayDirSun = normalize(-sunDir + viewDir);
    float specSun = pow(max(dot(normal, halfwayDirSun), 0.0), 32.0);
    vec3 specularSun = specularStrength * specSun * sunColor;

    // --- Point lights (same as before) ---
    vec3 lighting = diffuseSun + specularSun;
    for (int i = 0; i < ubo.numLights; i++) {
        // Diffuse
        vec3 lightDir = normalize(ubo.lights[i].position - fragWorldPos);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * ubo.lights[i].color;

        // Specular        
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
        vec3 specular = specularStrength * spec * ubo.lights[i].color;

        // Attenuation
        float distance = length(ubo.lights[i].position - fragWorldPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

        lighting += (diffuse + specular) * attenuation;
    }

    vec3 result = (ambient + lighting) * fragColor.rgb;
    outColor = vec4(result, fragColor.a);
}
