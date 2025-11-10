#pragma once

#include "Vertex.h"
#include "types.cpp"
#include <vector>
#include <glm/glm.hpp>

class Voxel {
public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    Voxel(const glm::vec3& center, float size, const glm::vec4& color) {
        float halfSize = size / 2.0f;

        // Define the 8 vertices of the voxel
        glm::vec3 positions[8] = {
            {center.x - halfSize, center.y - halfSize, center.z + halfSize}, // 0: Front-bottom-left
            {center.x + halfSize, center.y - halfSize, center.z + halfSize}, // 1: Front-bottom-right
            {center.x + halfSize, center.y + halfSize, center.z + halfSize}, // 2: Front-top-right
            {center.x - halfSize, center.y + halfSize, center.z + halfSize}, // 3: Front-top-left
            {center.x - halfSize, center.y - halfSize, center.z - halfSize}, // 4: Back-bottom-left
            {center.x + halfSize, center.y - halfSize, center.z - halfSize}, // 5: Back-bottom-right
            {center.x + halfSize, center.y + halfSize, center.z - halfSize}, // 6: Back-top-right
            {center.x - halfSize, center.y + halfSize, center.z - halfSize}  // 7: Back-top-left
        };

        // Define normals for each face
        glm::vec3 normals[6] = {
            { 0.0f,  0.0f,  1.0f}, // Front face
            { 0.0f,  0.0f, -1.0f}, // Back face
            { 1.0f,  0.0f,  0.0f}, // Right face
            {-1.0f,  0.0f,  0.0f}, // Left face
            { 0.0f,  1.0f,  0.0f}, // Top face
            { 0.0f, -1.0f,  0.0f}  // Bottom face
        };

        // Define texture coordinates for each vertex
        glm::vec2 texCoords[4] = {
            {0.0f, 1.0f}, // Bottom-left
            {1.0f, 1.0f}, // Bottom-right
            {1.0f, 0.0f}, // Top-right
            {0.0f, 0.0f}  // Top-left
        };
        
        // Define vertices for each face
        // Each face has 4 vertices
        unsigned int vertex_index = 0;

        float object_type = (float)ObjectType::Voxel;
        
        // Front face
        vertices.push_back({positions[0], texCoords[0], color, {0.0f, 0.0f}, object_type, normals[0]});
        vertices.push_back({positions[1], texCoords[1], color, {0.0f, 0.0f}, object_type, normals[0]});
        vertices.push_back({positions[2], texCoords[2], color, {0.0f, 0.0f}, object_type, normals[0]});
        vertices.push_back({positions[3], texCoords[3], color, {0.0f, 0.0f}, object_type, normals[0]});
        indices.insert(indices.end(), {vertex_index, vertex_index + 1, vertex_index + 2, vertex_index, vertex_index + 2, vertex_index + 3});
        vertex_index += 4;

        // Back face
        vertices.push_back({positions[4], texCoords[1], color, {0.0f, 0.0f}, object_type, normals[1]});
        vertices.push_back({positions[5], texCoords[0], color, {0.0f, 0.0f}, object_type, normals[1]});
        vertices.push_back({positions[6], texCoords[3], color, {0.0f, 0.0f}, object_type, normals[1]});
        vertices.push_back({positions[7], texCoords[2], color, {0.0f, 0.0f}, object_type, normals[1]});
        indices.insert(indices.end(), {vertex_index, vertex_index + 1, vertex_index + 2, vertex_index, vertex_index + 2, vertex_index + 3});
        vertex_index += 4;

        // Right face
        vertices.push_back({positions[1], texCoords[0], color, {0.0f, 0.0f}, object_type, normals[2]});
        vertices.push_back({positions[5], texCoords[1], color, {0.0f, 0.0f}, object_type, normals[2]});
        vertices.push_back({positions[6], texCoords[2], color, {0.0f, 0.0f}, object_type, normals[2]});
        vertices.push_back({positions[2], texCoords[3], color, {0.0f, 0.0f}, object_type, normals[2]});
        indices.insert(indices.end(), {vertex_index, vertex_index + 1, vertex_index + 2, vertex_index, vertex_index + 2, vertex_index + 3});
        vertex_index += 4;

        // Left face
        vertices.push_back({positions[4], texCoords[0], color, {0.0f, 0.0f}, object_type, normals[3]});
        vertices.push_back({positions[0], texCoords[1], color, {0.0f, 0.0f}, object_type, normals[3]});
        vertices.push_back({positions[3], texCoords[2], color, {0.0f, 0.0f}, object_type, normals[3]});
        vertices.push_back({positions[7], texCoords[3], color, {0.0f, 0.0f}, object_type, normals[3]});
        indices.insert(indices.end(), {vertex_index, vertex_index + 1, vertex_index + 2, vertex_index, vertex_index + 2, vertex_index + 3});
        vertex_index += 4;

        // Top face
        vertices.push_back({positions[3], texCoords[0], color, {0.0f, 0.0f}, object_type, normals[4]});
        vertices.push_back({positions[2], texCoords[1], color, {0.0f, 0.0f}, object_type, normals[4]});
        vertices.push_back({positions[6], texCoords[2], color, {0.0f, 0.0f}, object_type, normals[4]});
        vertices.push_back({positions[7], texCoords[3], color, {0.0f, 0.0f}, object_type, normals[4]});
        indices.insert(indices.end(), {vertex_index, vertex_index + 1, vertex_index + 2, vertex_index, vertex_index + 2, vertex_index + 3});
        vertex_index += 4;

        // Bottom face
        vertices.push_back({positions[4], texCoords[0], color, {0.0f, 0.0f}, object_type, normals[5]});
        vertices.push_back({positions[5], texCoords[1], color, {0.0f, 0.0f}, object_type, normals[5]});
        vertices.push_back({positions[1], texCoords[2], color, {0.0f, 0.0f}, object_type, normals[5]});
        vertices.push_back({positions[0], texCoords[3], color, {0.0f, 0.0f}, object_type, normals[5]});
        indices.insert(indices.end(), {vertex_index, vertex_index + 1, vertex_index + 2, vertex_index, vertex_index + 2, vertex_index + 3});
    }
};