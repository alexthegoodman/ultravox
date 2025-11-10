#pragma once

#include "Chunk.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <filesystem>
#include <queue>
#include <unordered_set>

// Custom hash for ChunkCoord
namespace std {
    template<>
    struct hash<Chunk::ChunkCoord> {
        size_t operator()(const Chunk::ChunkCoord& c) const {
            return ((hash<int>()(c.x) ^ (hash<int>()(c.y) << 1)) >> 1) ^ (hash<int>()(c.z) << 1);
        }
    };
}

class ChunkManager {
public:
    ChunkManager(const std::string& worldPath = "world_data") 
        : worldDataPath(worldPath),
          loadRadius(3),
          unloadRadius(5) {
        // Create world data directory if it doesn't exist
        std::filesystem::create_directories(worldDataPath);
        LOG("ChunkManager initialized with path: " + worldDataPath);
    }
    
    // Update which chunks should be loaded based on camera position
    void updateLoadedChunks(const glm::vec3& cameraPosition) {
        Chunk::ChunkCoord centerChunk = worldToChunkCoord(cameraPosition);
        
        // Queue chunks to load
        std::vector<Chunk::ChunkCoord> chunksToLoad;
        for (int x = -loadRadius; x <= loadRadius; ++x) {
            for (int y = -loadRadius; y <= loadRadius; ++y) {
                for (int z = -loadRadius; z <= loadRadius; ++z) {
                    Chunk::ChunkCoord coord{
                        centerChunk.x + x,
                        centerChunk.y + y,
                        centerChunk.z + z
                    };
                    
                    // Check if within sphere (not cube)
                    float dist = glm::length(glm::vec3(x, y, z));
                    if (dist <= loadRadius && loadedChunks.find(coord) == loadedChunks.end()) {
                        chunksToLoad.push_back(coord);
                    }
                }
            }
        }
        
        // Load new chunks
        for (const auto& coord : chunksToLoad) {
            loadChunk(coord);
        }
        
        // Unload distant chunks
        std::vector<Chunk::ChunkCoord> chunksToUnload;
        for (const auto& [coord, chunk] : loadedChunks) {
            float dist = glm::length(glm::vec3(
                coord.x - centerChunk.x,
                coord.y - centerChunk.y,
                coord.z - centerChunk.z
            ));
            
            if (dist > unloadRadius) {
                chunksToUnload.push_back(coord);
            }
        }
        
        for (const auto& coord : chunksToUnload) {
            unloadChunk(coord);
        }
    }
    
    // Get chunk at coordinate (returns nullptr if not loaded)
    Chunk* getChunk(const Chunk::ChunkCoord& coord) {
        auto it = loadedChunks.find(coord);
        return (it != loadedChunks.end()) ? it->second.get() : nullptr;
    }
    
    // Get chunk at world position
    Chunk* getChunkAtWorldPos(const glm::vec3& worldPos) {
        return getChunk(worldToChunkCoord(worldPos));
    }
    
    // Set voxel at world position
    void setVoxelWorld(const glm::vec3& worldPos, const Chunk::VoxelData& data) {
        Chunk::ChunkCoord chunkCoord = worldToChunkCoord(worldPos);
        Chunk* chunk = getChunk(chunkCoord);
        
        if (!chunk) {
            // Create chunk if it doesn't exist
            chunk = loadOrCreateChunk(chunkCoord);
        }
        
        // Convert to local chunk coordinates
        glm::ivec3 localPos = worldToLocalVoxel(worldPos, chunkCoord);
        chunk->setVoxel(localPos.x, localPos.y, localPos.z, data);
        
        // Mark chunk as modified
        modifiedChunks.insert(chunkCoord);
    }
    
    // Get voxel at world position
    Chunk::VoxelData getVoxelWorld(const glm::vec3& worldPos) {
        Chunk::ChunkCoord chunkCoord = worldToChunkCoord(worldPos);
        Chunk* chunk = getChunk(chunkCoord);
        
        if (!chunk) return Chunk::VoxelData(); // Return air
        
        glm::ivec3 localPos = worldToLocalVoxel(worldPos, chunkCoord);
        return chunk->getVoxel(localPos.x, localPos.y, localPos.z);
    }
    
    // Rebuild meshes for dirty chunks
    void rebuildDirtyChunks() {
        for (auto& [coord, chunk] : loadedChunks) {
            if (chunk->isDirty()) {
                chunk->rebuildMesh();
            }
        }
    }
    
    // Save all modified chunks to disk
    void saveModifiedChunks() {
        for (const auto& coord : modifiedChunks) {
            auto it = loadedChunks.find(coord);
            if (it != loadedChunks.end()) {
                saveChunk(coord, it->second.get());
            }
        }
        modifiedChunks.clear();
        LOG("Saved all modified chunks");
    }
    
    // Save all loaded chunks (useful on shutdown)
    void saveAllChunks() {
        for (const auto& [coord, chunk] : loadedChunks) {
            saveChunk(coord, chunk.get());
        }
        LOG("Saved all loaded chunks");
    }
    
    // Get all loaded chunks
    const std::unordered_map<Chunk::ChunkCoord, std::unique_ptr<Chunk>>& getLoadedChunks() const {
        return loadedChunks;
    }

    void generateTerrain(Chunk* chunk) {
        const Chunk::ChunkCoord& coord = chunk->getCoordinate();
        glm::vec3 worldPos = chunk->getWorldPosition();
        
        // Only generate terrain for chunks that touch y=0
        if (coord.y != 0) return;
        
        // Simple flat terrain at y=0
        for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
            for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                // Just place a single voxel at y=0
                // glm::vec4 color(0.3f, 0.6f, 0.2f, 1.0f); // Green grass color
                float colorVariation = static_cast<float>(z + x) / Chunk::CHUNK_SIZE;
                glm::vec4 color(0.3f + colorVariation * 0.5f, 0.6f, 0.2f, 1.0f);
                chunk->setVoxel(x, 0, z, Chunk::VoxelData(color, 1));
            }
        }
    }

    // Public function for the ImGUI button
    void generateFlatLandscape() {
        // 1. Define the dimensions for 100x100 area at y=0
        // CHUNK_SIZE = 32. 100/32 = 3.125. We need 4 chunks (0, 1, 2, 3).
        const int NUM_CHUNKS_X = 4;
        const int NUM_CHUNKS_Z = 4;
        const int CHUNK_Y = 0; // The layer where the terrain will sit
        
        LOG("Starting generation and saving of 4x4 flat chunk landscape.");
        
        // 2. Iterate through the required chunk coordinates
        for (int cx = 0; cx < NUM_CHUNKS_X; ++cx) {
            for (int cz = 0; cz < NUM_CHUNKS_Z; ++cz) {
                
                Chunk::ChunkCoord coord{cx, CHUNK_Y, cz};
                
                // A. Create a new, temporary Chunk object (starts as all air)
                auto newChunk = std::make_unique<Chunk>(coord);
                
                // B. Generate the flat terrain using the existing logic
                // NOTE: Your existing generateTerrain function only populates y=0.
                generateTerrain(newChunk.get()); 
                
                // C. Force a mesh rebuild to ensure the 'isEmpty' flag is updated
                // (Crucial for efficient loading/saving if empty chunks are skipped)
                newChunk->rebuildMesh(); 
                
                // D. Save the chunk directly to disk
                // We use newChunk.get() because saveChunk expects a raw pointer.
                saveChunk(coord, newChunk.get());
            }
        }

        // TODO: load in from file now or no?

        LOG("Flat landscape generation complete and saved to disk.");
    }
    
    // Configuration
    void setLoadRadius(int radius) { loadRadius = radius; }
    void setUnloadRadius(int radius) { unloadRadius = radius; }
    int getLoadRadius() const { return loadRadius; }
    int getUnloadRadius() const { return unloadRadius; }

private:
    std::unordered_map<Chunk::ChunkCoord, std::unique_ptr<Chunk>> loadedChunks;
    std::unordered_set<Chunk::ChunkCoord> modifiedChunks;
    std::string worldDataPath;
    int loadRadius;
    int unloadRadius;
    
    // Convert world position to chunk coordinate
    Chunk::ChunkCoord worldToChunkCoord(const glm::vec3& worldPos) const {
        return Chunk::ChunkCoord{
            static_cast<int>(floor(worldPos.x / (Chunk::CHUNK_SIZE * Chunk::VOXEL_SIZE))),
            static_cast<int>(floor(worldPos.y / (Chunk::CHUNK_SIZE * Chunk::VOXEL_SIZE))),
            static_cast<int>(floor(worldPos.z / (Chunk::CHUNK_SIZE * Chunk::VOXEL_SIZE)))
        };
    }
    
    // Convert world position to local voxel position within chunk
    glm::ivec3 worldToLocalVoxel(const glm::vec3& worldPos, const Chunk::ChunkCoord& chunkCoord) const {
        glm::vec3 chunkWorldPos(
            chunkCoord.x * Chunk::CHUNK_SIZE * Chunk::VOXEL_SIZE,
            chunkCoord.y * Chunk::CHUNK_SIZE * Chunk::VOXEL_SIZE,
            chunkCoord.z * Chunk::CHUNK_SIZE * Chunk::VOXEL_SIZE
        );
        
        glm::vec3 local = (worldPos - chunkWorldPos) / Chunk::VOXEL_SIZE;
        return glm::ivec3(
            static_cast<int>(floor(local.x)),
            static_cast<int>(floor(local.y)),
            static_cast<int>(floor(local.z))
        );
    }
    
    // Get file path for chunk
    std::string getChunkFilePath(const Chunk::ChunkCoord& coord) const {
        return worldDataPath + "/chunk_" + 
               std::to_string(coord.x) + "_" + 
               std::to_string(coord.y) + "_" + 
               std::to_string(coord.z) + ".dat";
    }
    
    // Load chunk from disk or create new
    Chunk* loadOrCreateChunk(const Chunk::ChunkCoord& coord) {
        auto chunk = std::make_unique<Chunk>(coord);
        std::string filePath = getChunkFilePath(coord);
        
        if (std::filesystem::exists(filePath)) {
            std::ifstream file(filePath, std::ios::binary);
            if (file.is_open() && chunk->loadFromBinary(file)) {
                LOG("Loaded chunk from disk: " + filePath);
            }
            file.close();
        } 
        // no desire for automatic terrain generation, only via button click(s)
        // else {
        //     // Generate new terrain for this chunk
        //     generateTerrain(chunk.get());
        //     LOG("Generated new chunk: " + filePath);
        // }
        
        Chunk* ptr = chunk.get();
        loadedChunks[coord] = std::move(chunk);
        return ptr;
    }
    
    // Load chunk (from disk or generate)
    void loadChunk(const Chunk::ChunkCoord& coord) {
        if (loadedChunks.find(coord) != loadedChunks.end()) return;
        loadOrCreateChunk(coord);
    }
    
    // Unload chunk (save first if modified)
    void unloadChunk(const Chunk::ChunkCoord& coord) {
        auto it = loadedChunks.find(coord);
        if (it == loadedChunks.end()) return;
        
        // Save if modified
        if (modifiedChunks.find(coord) != modifiedChunks.end()) {
            saveChunk(coord, it->second.get());
            modifiedChunks.erase(coord);
        }
        
        loadedChunks.erase(it);
        LOG("Unloaded chunk: " + std::to_string(coord.x) + "," + 
            std::to_string(coord.y) + "," + std::to_string(coord.z));
    }
    
    // Save chunk to disk
    void saveChunk(const Chunk::ChunkCoord& coord, Chunk* chunk) {
        std::string filePath = getChunkFilePath(coord);
        std::ofstream file(filePath, std::ios::binary);
        
        if (file.is_open()) {
            chunk->saveToBinary(file);
            file.close();
        } else {
            LOG("ERROR: Could not save chunk to " + filePath);
        }
    }

    
};