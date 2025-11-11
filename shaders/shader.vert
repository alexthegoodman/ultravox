#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoords;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inGradientCoords;
layout(location = 4) in float inObjectType;
layout(location = 5) in vec3 inNormal;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    // mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

void main() {
    // gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * push.model * vec4(inPosition, 1.0);
    // gl_Position = ubo.proj * ubo.view * vec4(inPosition, 1.0);
    fragColor = inColor;
    // fragColor = vec4(abs(push.model[3].xyz) * 0.02, 1.0);
}
