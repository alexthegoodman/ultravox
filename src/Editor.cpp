#pragma once

#include "Vertex.h"
#include "Chunk.h"
#include "ChunkManager.h"

#include <vector>

class Editor {
public:
    // management
    ChunkManager chunkManager;

    // state
    bool isPlayingPreview = false;

    Editor() {
        // LOG("Starting Editor");
        
        // editor settings allow for more to visible
        chunkManager.setLoadRadius(10);  // Load chunks (by center of chunk) within 5 units radius of camera position
        chunkManager.setUnloadRadius(16); // Unload chunks (by center of chunk) within 8 units radius of camera position
    }

    void startPlayingPreview() {
        isPlayingPreview = true;
        
        // restrict range to improve gameplay performance
        chunkManager.setLoadRadius(5);
        chunkManager.setUnloadRadius(8);
    }

    void stopPlayingPreview() {
        isPlayingPreview = false;
        chunkManager.setLoadRadius(10);
        chunkManager.setUnloadRadius(16);
    }
};