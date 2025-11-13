#pragma once

#include "Vertex.h"
#include "Chunk.h"
#include "ChunkManager.h"
#include "PlayerCharacter.h"
#include "Sphere.h"
#include "Light.h"

#include <vector>
#include <memory>

class Editor {
public:
    // management
    ChunkManager chunkManager;
    std::unique_ptr<PlayerCharacter> playerCharacter;
    std::vector<PointLight> lights;

    // state
    bool isPlayingPreview = false;
    bool isPainting = false;
    bool isPaintingComponent = false;
    ComponentType isPaintingComponentType = ComponentType::Tree;
    bool isPaintingItem = false;
    ItemType isPaintingItemType = ItemType::Apple; // defaults to Apple

    Editor() {
        // LOG("Starting Editor");
        
        // editor settings allow for more to visible
        chunkManager.setLoadRadius(4);
        chunkManager.setUnloadRadius(8);
    }

    void startPlayingPreview() {
        isPlayingPreview = true;
        
        // restrict range to improve gameplay performance
        // chunkManager.setLoadRadius(2);
        // chunkManager.setUnloadRadius(4);
    }

    void stopPlayingPreview() {
        isPlayingPreview = false;
        // chunkManager.setLoadRadius(5);
        // chunkManager.setUnloadRadius(8);
    }
};