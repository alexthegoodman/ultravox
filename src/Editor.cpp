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

    // Texture IDs for components
    // int houseWallTextureId = 0;
    // int houseRoofTextureId = 0;
    // int houseDoorTextureId = 0;
    // int treeTrunkTextureId = 0;
    // int treeLeavesTextureId = 0;
    // int domeTextureId = 0;
    // int domeDebrisTextureId = 0;

    int componentTexture1Id = 0;
    int componentTexture2Id = 0;
    int componentTexture3Id = 0;
    int componentTexture4Id = 0;

    // Texture IDs for terrain generation
    int terrainGrassTextureId = 0;
    int terrainDirtTextureId = 0;
    int terrainStoneTextureId = 0;

    Editor() {
        // LOG("Starting Editor");
        
        // editor settings allow for more to visible
        chunkManager.setLoadRadius(8);
        chunkManager.setUnloadRadius(12);
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