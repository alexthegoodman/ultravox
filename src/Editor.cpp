#pragma once

#include "Vertex.h"
#include "Chunk.h"
#include "ChunkManager.h"

#include <vector>

class Editor {
public:
    // std::vector<Voxel> voxels;
    
    // Combined mesh data
    std::vector<Vertex> vertexCache;
    std::vector<uint32_t> indiceCache;

    ChunkManager chunkManager;

    Editor() {
        // LOG("Starting Editor");
        
        chunkManager.setLoadRadius(5);  // Load chunks within 5 chunk radius
        chunkManager.setUnloadRadius(8); // Unload beyond 8 chunk radius
    }

    const std::vector<Vertex>& getVertices() const {
        return vertexCache;
    }

    const std::vector<uint32_t>& getIndices() const {
        return indiceCache;
    }
};