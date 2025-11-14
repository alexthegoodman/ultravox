#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include "Octree.h" // For Vector3 definition
#include "Vertex.h"
#include <vector>

inline glm::vec2 toNDC(float x, float y, float width, float height) {
    float ndcX = (x / width) * 2.0f - 1.0f;
    float ndcY = -((y / height) * 2.0f - 1.0f);
    return { ndcX, ndcY };
}


inline glm::vec2 fromNDC(float ndcX, float ndcY, float width, float height) {
    float x = ((ndcX + 1.0f) / 2.0f) * width;
    float y = ((-ndcY + 1.0f) / 2.0f) * height;
    return { x, y };
}

inline float toSystemScale(float humanDim, float windowDim) {
    return (humanDim / windowDim) * 2.0f;
}

inline float fromSystemScale(float scaleDim, float windowDim) {
    return (scaleDim / 2.0f) * windowDim;
}

// --- HSL to RGB Conversion Helper Function ---
// This is a standard HSL/HSV to RGB conversion often used in graphics.
// It converts a Hue (h) from [0, 1] to an RGB color.
glm::vec4 HSLtoRGB(float h, float s, float l, float a) {
    // If saturation is 0, it's a shade of gray
    if (s == 0.0f) {
        return glm::vec4(l, l, l, a);
    }
    
    auto hue2rgb = [](float p, float q, float t) {
        if (t < 0.0f) t += 1.0f;
        if (t > 1.0f) t -= 1.0f;
        if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
        if (t < 1.0f/2.0f) return q;
        if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
        return p;
    };
    
    float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
    float p = 2.0f * l - q;
    
    float r = hue2rgb(p, q, h + 1.0f/3.0f);
    float g = hue2rgb(p, q, h);
    float b = hue2rgb(p, q, h - 1.0f/3.0f);
    
    return glm::vec4(r, g, b, a);
}

// Conversion from glm::vec3 to Vector3
inline Vector3 toCustomVector3(const glm::vec3& v) {
    return Vector3(v.x, v.y, v.z);
}

inline void createSphereMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int latitudeSegments, int longitudeSegments, float radius) {
    vertices.clear();
    indices.clear();

    for (int i = 0; i <= latitudeSegments; ++i) {
        float theta = i * M_PI / latitudeSegments;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (int j = 0; j <= longitudeSegments; ++j) {
            float phi = j * 2 * M_PI / longitudeSegments;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            Vertex vertex;
            vertex.position.x = radius * cosPhi * sinTheta;
            vertex.position.y = radius * cosTheta;
            vertex.position.z = radius * sinPhi * sinTheta;
            vertex.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // Red color for the sphere
            vertices.push_back(vertex);
        }
    }

    for (int i = 0; i < latitudeSegments; ++i) {
        for (int j = 0; j < longitudeSegments; ++j) {
            int first = (i * (longitudeSegments + 1)) + j;
            int second = first + longitudeSegments + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
}

