#pragma once

#include "Vertex.h"
#include "Chunk.h"
#include "ChunkManager.h"
#include "PlayerCharacter.h"
#include "Sphere.h"

#include <vector>
#include <memory>

class Editor {
public:
    // management
    ChunkManager chunkManager;
    std::unique_ptr<PlayerCharacter> playerCharacter;

    // state
    bool isPlayingPreview = false;

    Editor() {
        // LOG("Starting Editor");
        
        // editor settings allow for more to visible
        chunkManager.setLoadRadius(5);
        chunkManager.setUnloadRadius(8);
    }

    void startPlayingPreview() {
        isPlayingPreview = true;
        
        // restrict range to improve gameplay performance
        chunkManager.setLoadRadius(2);
        chunkManager.setUnloadRadius(4);
    }

    void stopPlayingPreview() {
        isPlayingPreview = false;
        chunkManager.setLoadRadius(5);
        chunkManager.setUnloadRadius(8);
    }
};