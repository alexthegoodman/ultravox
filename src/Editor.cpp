#pragma once

#include <vector>

// since I don't prefer header files currently, I am including needed files in main.cpp for use in other files :)
// #include "Voxel.cpp"
// #include "Logger.cpp"

// #define LOG(message) Logger::getInstance().log(message)

class Editor {
public:
    std::vector<Voxel> voxels;
    
    // Combined mesh data
    std::vector<Vertex> vertexCache;
    std::vector<uint32_t> indiceCache;

    Editor() {
        LOG("Starting Editor");

        // Create a 1000x1000 grid of voxels as base landscape
        const int gridSize = 2;
        const float voxelSize = 1.0f;
        
        voxels.reserve(gridSize * gridSize);
        
        for (int x = 0; x < gridSize; ++x) {
            for (int z = 0; z < gridSize; ++z) {
                glm::vec3 position(
                    x * voxelSize - (gridSize * voxelSize) / 2.0f,
                    0.0f,
                    z * voxelSize - (gridSize * voxelSize) / 2.0f
                );
                
                // Vary color slightly for visual interest
                float colorVariation = (x + z) % 10 / 10.0f * 0.2f;
                glm::vec4 color(0.8f + colorVariation, 0.5f, 0.2f, 1.0f);
                
                voxels.emplace_back(position, voxelSize, color);
            }
        }

        LOG("Rebuilding Mesh Cache");
        
        // Build combined mesh
        rebuildMesh();
    }

    void rebuildMesh() {
        vertexCache.clear();
        indiceCache.clear();
        
        uint32_t vertexOffset = 0;
        
        for (const auto& voxel : voxels) {
            // Add vertices
            vertexCache.insert(
                vertexCache.end(),
                voxel.vertices.begin(),
                voxel.vertices.end()
            );
            
            // Add indices with offset
            for (uint32_t index : voxel.indices) {
                indiceCache.push_back(index + vertexOffset);
            }
            
            vertexOffset += static_cast<uint32_t>(voxel.vertices.size());
        }
    }

    const std::vector<Vertex>& getVertices() const {
        return vertexCache;
    }

    const std::vector<uint32_t>& getIndices() const {
        return indiceCache;
    }
};