#pragma once

#include "Cube.cpp"
#include <vector>

class Editor {
public:
    std::vector<Cube> cubes;

    Editor() {
        // Add a default cube
        cubes.emplace_back(glm::vec3(0.0f, 0.0f, 0.0f), 1.0f, glm::vec4(1.0f, 0.5f, 0.2f, 1.0f));
    }

    const std::vector<Vertex>& getVertices() const {
        // For now, just return vertices of the first cube.
        // A more robust implementation would combine vertices from all objects.
        if (!cubes.empty()) {
            return cubes[0].vertices;
        }
        static std::vector<Vertex> empty;
        return empty;
    }

    const std::vector<uint32_t>& getIndices() const {
        // For now, just return indices of the first cube.
        if (!cubes.empty()) {
            return cubes[0].indices;
        }
        static std::vector<uint32_t> empty;
        return empty;
    }
};
