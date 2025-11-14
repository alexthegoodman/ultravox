#pragma once

#include "Vertex.h"
#include "Chunk.h"
#include "ChunkManager.h"
#include "PlayerCharacter.h"
#include "Sphere.h"
#include "Light.h"
#include "TextureManager.h" // Include TextureManager header
#include "types.h"

#include <vector>
#include <memory>

class Editor {
public:
    // management
    ChunkManager chunkManager;
    std::unique_ptr<PlayerCharacter> playerCharacter;
    std::vector<PointLight> lights;
    TextureManager* textureManager = nullptr; // Pointer to the TextureManager

    // state
    bool isPlayingPreview = false;
    bool isPainting = false;
    bool isPaintingComponent = false;
    ComponentType isPaintingComponentType = ComponentType::Tree;
    bool isPaintingItem = false;
    ItemType isPaintingItemType = ItemType::Apple; // defaults to Apple
    int selectedTextureId = 0; // Index of the currently selected texture

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