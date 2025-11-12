#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <unordered_map>
#include <memory>
#include <string>
#include <filesystem>
#include <queue>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <iomanip>

#include "ChunkManager.h"
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp> // For glm::length2

#include "Chunk.h"
#include "Logger.h"
#include "helpers.h"
#include "Octree.h" // Include Octree implementation
#include "PhysicsSystem.h" // For PhysicsSystem::RayCastResult

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
    Octree<Chunk::PhysicsVoxelData> physicsOctree; 

    ChunkManager(const std::string& worldPath = "world_data") 
        : worldDataPath(worldPath),
          loadRadius(3),
          unloadRadius(5),
          physicsOctree() { // Initialize Octree with a large enough bounds
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
                    // if (dist <= loadRadius && loadedChunks.find(coord) == loadedChunks.end()) {
                    //     chunksToLoad.push_back(coord);
                    // }

                    if (dist <= loadRadius &&
                        loadedChunks.find(coord) == loadedChunks.end() &&
                        missingChunks.find(coord) == missingChunks.end()) {
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
    // void setVoxelWorld(const glm::vec3& worldPos, const Chunk::VoxelData& data) {
    //     Chunk::ChunkCoord chunkCoord = worldToChunkCoord(worldPos);
    //     Chunk* chunk = getChunk(chunkCoord);
        
    //     if (!chunk) {
    //         // Create chunk if it doesn't exist
    //         chunk = loadOrCreateChunk(chunkCoord);
    //     }
        
    //     // Convert to local chunk coordinates
    //     glm::ivec3 localPos = worldToLocalVoxel(worldPos, chunkCoord);
    //     chunk->setVoxel(localPos.x, localPos.y, localPos.z, data);
        
    //     // Mark chunk as modified
    //     modifiedChunks.insert(chunkCoord);
    // }

    void setVoxelWorld(const glm::vec3& worldPos, const Chunk::VoxelData& data) {
        Chunk::ChunkCoord chunkCoord = worldToChunkCoord(worldPos);
        Chunk* chunk = getChunk(chunkCoord);

        if (!chunk) {
            chunk = loadChunk2(chunkCoord);
            if (!chunk) {
                // Create an empty chunk to hold new voxel edits
                chunk = createEmptyChunk(chunkCoord);
                
                // LOG("Created new chunk at " + std::to_string(chunkCoord.x) + ", " + std::to_string(chunkCoord.y) + ", " + std::to_string(chunkCoord.z));
            }
        }

        glm::ivec3 localPos = worldToLocalVoxel(worldPos, chunkCoord);
        chunk->setVoxel(localPos.x, localPos.y, localPos.z, data);
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

    // void generateTerrain(Chunk* chunk) {
    //     const Chunk::ChunkCoord& coord = chunk->getCoordinate();
    //     glm::vec3 worldPos = chunk->getWorldPosition();
        
    //     // Only generate terrain for chunks that touch y=0
    //     if (coord.y != 0) return;
        
    //     // Simple flat terrain at y=0
    //     for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
    //         for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
    //             // Just place a single voxel at y=0
    //             // glm::vec4 color(0.3f, 0.6f, 0.2f, 1.0f); // Green grass color
    //             float colorVariation = static_cast<float>(z + x) / Chunk::CHUNK_SIZE;
    //             glm::vec4 color(0.3f + colorVariation * 0.5f, 0.6f, 0.2f, 1.0f);
    //             chunk->setVoxel(x, 0, z, Chunk::VoxelData(color, 1));
    //         }
    //     }
    // }

    void generateTerrain(Chunk* chunk) {
        const Chunk::ChunkCoord& coord = chunk->getCoordinate();
        glm::vec3 chunkWorldPos = chunk->getWorldPosition();
        
        // Only generate terrain for chunks that touch y=0
        if (coord.y != 0) return;
        
        // Simple flat terrain at y=0
        for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
            for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                
                // Calculate a color index based on the x and z coordinates
                // We use the full extent of the chunk for the color range (0 to 2*CHUNK_SIZE - 2)
                float totalIndex = static_cast<float>(x + z);
                float maxIndex = static_cast<float>(Chunk::CHUNK_SIZE * 2 - 2);
                
                // Map the index to a Hue value (0.0 to 1.0) for the rainbow
                float hue = totalIndex / maxIndex; // 0.0 (Red) -> 1.0 (Red/Violet)
                
                // Set Saturation and Lightness for vibrant colors
                float saturation = 1.0f; // Full saturation
                float lightness = 0.5f;  // Mid-range lightness for pure hues
                
                // Convert HSL to an RGB color vector
                glm::vec4 color = HSLtoRGB(hue, saturation, lightness, 1.0f);
                
                // Place a single voxel at y=0 with the rainbow color
                chunk->setVoxel(x, 0, z, Chunk::VoxelData(color, 1));

                // Insert physics voxel data into the octree
                glm::vec3 voxelWorldPos = chunkWorldPos + glm::vec3(x, 0, z) * Chunk::VOXEL_SIZE;
                Chunk::PhysicsVoxelData physicsData(voxelWorldPos, Chunk::VOXEL_SIZE, 1);
                physicsOctree.insert(Vector3(voxelWorldPos.x, voxelWorldPos.y, voxelWorldPos.z), physicsData);
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

    // Helper function to read a binary chunk file and write the contents to a text file
    void exportChunkDataToTextFile(const std::string& binaryFilePath, const std::string& textExportPath) {
        std::ifstream binaryFile(binaryFilePath, std::ios::binary);
        std::ofstream textFile(textExportPath);

        if (!binaryFile.is_open()) {
            std::cerr << "ERROR: Could not open binary file: " << binaryFilePath << std::endl;
            return;
        }
        if (!textFile.is_open()) {
            std::cerr << "ERROR: Could not create output file: " << textExportPath << std::endl;
            return;
        }

        textFile << "--- CHUNK DATA INSPECTION REPORT ---" << std::endl;
        textFile << "Source Binary: " << binaryFilePath << std::endl;
        textFile << "-----------------------------------" << std::endl;

        // 1. Read and print ChunkCoord (3 ints)
        Chunk::ChunkCoord coord;
        if (binaryFile.read(reinterpret_cast<char*>(&coord), sizeof(Chunk::ChunkCoord))) {
            textFile << "  [HEADER] Coordinate: (" << coord.x << ", " << coord.y << ", " << coord.z << ")" << std::endl;
        } else {
            textFile << "ERROR: Could not read ChunkCoord header. File might be truncated." << std::endl;
            return;
        }

        // 2. Read and print isEmpty flag (1 bool)
        bool isEmpty = false;
        if (binaryFile.read(reinterpret_cast<char*>(&isEmpty), sizeof(bool))) {
            textFile << "  [HEADER] Is Empty: " << (isEmpty ? "TRUE" : "FALSE") << std::endl;
        } else {
            textFile << "ERROR: Could not read isEmpty flag." << std::endl;
            return;
        }

        // 3. Read and print Voxel Data (if not empty)
        if (!isEmpty) {
            textFile << "  [DATA] Voxel data present. Size: " << Chunk::CHUNK_SIZE << "^3 voxels." << std::endl;
            
            // Setup for readable output
            textFile << std::fixed << std::setprecision(4); // Increased precision for color verification
            
            int solidCount = 0;
            int maxVoxels = Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE;
            
            for (int i = 0; i < maxVoxels; ++i) {
                Chunk::VoxelData data;
                
                if (binaryFile.read(reinterpret_cast<char*>(&data), sizeof(Chunk::VoxelData))) {
                    if (data.type != 0) {
                        solidCount++;
                        
                        // Convert flat index back to local (x, y, z)
                        int x = i % Chunk::CHUNK_SIZE;
                        int y = (i / Chunk::CHUNK_SIZE) % Chunk::CHUNK_SIZE;
                        int z = i / (Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE);
                        
                        // Write solid voxel details
                        textFile << "    (" << std::setw(2) << x << "," << std::setw(2) << y << "," << std::setw(2) << z << ") " 
                                << "Type: " << (int)data.type 
                                << ", Color: (" << data.color.r << ", " << data.color.g << ", " << data.color.b << ", " << data.color.a << ")" 
                                << std::endl;
                    }
                } else {
                    textFile << "ERROR: Failed to read all voxel data. File truncated after " << i << " voxels." << std::endl;
                    break;
                }
            }
            textFile << "  [SUMMARY] Total solid voxels read: " << solidCount << std::endl;
        }

        textFile << "----------------- END OF REPORT -----------------" << std::endl;
        
        // Close both files
        binaryFile.close();
        textFile.close();
        std::cout << "Successfully exported data inspection to: " << textExportPath << std::endl;
    }
    
    // Configuration

    // Set the radius (not in world units, but in number of chunks)
    void setLoadRadius(int radius) { loadRadius = radius; }

    // Set the radius (not in world units, but in number of chunks)
    void setUnloadRadius(int radius) { unloadRadius = radius; }
    
    // Get the radius (not in world units, but in number of chunks)
    int getLoadRadius() const { return loadRadius; }

    // Raycasting method
    PhysicsSystem::RayCastResult castRay(const glm::vec3& origin, const glm::vec3& direction);

    

private:
    std::unordered_map<Chunk::ChunkCoord, std::unique_ptr<Chunk>> loadedChunks;
    std::unordered_set<Chunk::ChunkCoord> modifiedChunks;
    std::unordered_set<Chunk::ChunkCoord> missingChunks;
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
    Chunk* loadChunk2(const Chunk::ChunkCoord& coord) {
        auto chunk = std::make_unique<Chunk>(coord);
        std::string filePath = getChunkFilePath(coord);
        
        bool loadedFromDisk = false;
        if (std::filesystem::exists(filePath)) {
            std::ifstream file(filePath, std::ios::binary);
            if (file.is_open() && chunk->loadFromBinary(file)) {
                LOG("Loaded chunk from disk: " + filePath);
                loadedFromDisk = true;
            }
            file.close();
        } 
        
        if (loadedFromDisk) {
            // If loaded from disk, populate physicsOctree with existing solid voxels
            glm::vec3 chunkWorldPos = chunk->getWorldPosition();
            for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
                for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
                    for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                        if (chunk->isSolid(x, y, z)) {
                            glm::vec3 voxelWorldPos = chunkWorldPos + glm::vec3(x, y, z) * Chunk::VOXEL_SIZE;
                            Chunk::PhysicsVoxelData physicsData(voxelWorldPos, Chunk::VOXEL_SIZE, chunk->getVoxel(x, y, z).type);
                            physicsOctree.insert(Vector3(voxelWorldPos.x, voxelWorldPos.y, voxelWorldPos.z), physicsData);
                        }
                    }
                }
            }
        } else {
            missingChunks.insert(coord); // Cache the miss
            return nullptr;
        }

        // we dont want this, we are not doing infinite worlds, just loading from disk
        // else {
        //     // Generate new terrain for this chunk (which also populates the octree)
        //     generateTerrain(chunk.get());
        //     LOG("Generated new chunk: " + filePath);
        // }
        
        Chunk* ptr = chunk.get();
        loadedChunks[coord] = std::move(chunk);
        return ptr;
    }

    // Chunk* loadChunk2(const Chunk::ChunkCoord& coord) {
    //     std::string filePath = getChunkFilePath(coord);

    //     if (!std::filesystem::exists(filePath)) {
    //         // No chunk file exists — don't create one
    //         // LOG("Chunk file not found: " + filePath);
    //         return nullptr;
    //     }

    //     auto chunk = std::make_unique<Chunk>(coord);
    //     bool loadedFromDisk = false;

    //     std::ifstream file(filePath, std::ios::binary);
    //     if (file.is_open() && chunk->loadFromBinary(file)) {
    //         LOG("Loaded chunk from disk: " + filePath);
    //         loadedFromDisk = true;
    //     }
    //     file.close();

    //     if (!loadedFromDisk) {
    //         LOG("Failed to load chunk from file: " + filePath);
    //         return nullptr;
    //     }

    //     // Populate physicsOctree with solid voxels
    //     glm::vec3 chunkWorldPos = chunk->getWorldPosition();
    //     for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
    //         for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
    //             for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
    //                 if (chunk->isSolid(x, y, z)) {
    //                     glm::vec3 voxelWorldPos = chunkWorldPos + glm::vec3(x, y, z) * Chunk::VOXEL_SIZE;
    //                     Chunk::PhysicsVoxelData physicsData(voxelWorldPos, Chunk::VOXEL_SIZE, chunk->getVoxel(x, y, z).type);
    //                     physicsOctree.insert(Vector3(voxelWorldPos.x, voxelWorldPos.y, voxelWorldPos.z), physicsData);
    //                 }
    //             }
    //         }
    //     }

    //     Chunk* ptr = chunk.get();
    //     loadedChunks[coord] = std::move(chunk);
    //     return ptr;
    // }
    
    Chunk* createEmptyChunk(const Chunk::ChunkCoord& coord) {
        // Safety check: avoid overwriting existing chunks
        if (loadedChunks.find(coord) != loadedChunks.end()) {
            LOG("createEmptyChunk: Chunk already exists at coord " 
                + std::to_string(coord.x) + ", " 
                + std::to_string(coord.y) + ", " 
                + std::to_string(coord.z));
            return loadedChunks[coord].get();
        }

        // Allocate and initialize a new blank chunk
        auto chunk = std::make_unique<Chunk>(coord);

        // Optionally, ensure the chunk voxel data starts empty
        chunk->fillVoxels(Chunk::VoxelData(glm::vec4(0.0f), 0)); // type 0 = air

        // Update physics (optional — if empty, nothing to insert)
        // But you might want to register its bounding box in the octree for spatial queries
        glm::vec3 chunkWorldPos = chunk->getWorldPosition();
        // (You can skip adding to physicsOctree unless you want empty chunks to exist physically)

        // Mark as modified so that it will be saved when the user finishes painting
        modifiedChunks.insert(coord);
        missingChunks.erase(coord);

        // Store in loadedChunks map
        Chunk* ptr = chunk.get();
        loadedChunks[coord] = std::move(chunk);

        LOG("Created new empty chunk at " 
            + std::to_string(coord.x) + ", " 
            + std::to_string(coord.y) + ", " 
            + std::to_string(coord.z));

        return ptr;
    }

    // Load chunk (from disk or generate)
    void loadChunk(const Chunk::ChunkCoord& coord) {
        if (loadedChunks.find(coord) != loadedChunks.end()) return;
        loadChunk2(coord);
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
        
        // Remove physics voxel data from the octree
        Chunk* chunk = it->second.get();
        glm::vec3 chunkWorldPos = chunk->getWorldPosition();
        for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
            for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
                for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                    if (chunk->isSolid(x, y, z)) {
                        glm::vec3 voxelWorldPos = chunkWorldPos + glm::vec3(x, y, z) * Chunk::VOXEL_SIZE;
                        // The predicate ensures we remove the correct PhysicsVoxelData if multiple exist at the same position (though for voxels, it should be unique)
                        physicsOctree.remove(Vector3(voxelWorldPos.x, voxelWorldPos.y, voxelWorldPos.z), 
                                             [&](const Chunk::PhysicsVoxelData& data){
                                                 return data.worldPosition == voxelWorldPos && data.type == chunk->getVoxel(x,y,z).type;
                                             });
                    }
                }
            }
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


// Helper function for ray-AABB intersection
// Returns true if ray intersects AABB, and sets t_min, hit_normal
bool intersectRayAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                      const glm::vec3& aabbMin, const glm::vec3& aabbMax,
                      float& t_min, glm::vec3& hit_normal) {
    glm::vec3 invDir = 1.0f / rayDirection;
    glm::vec3 t0 = (aabbMin - rayOrigin) * invDir;
    glm::vec3 t1 = (aabbMax - rayOrigin) * invDir;

    glm::vec3 t_min_vec = glm::min(t0, t1);
    glm::vec3 t_max_vec = glm::max(t0, t1);

    float t_enter = glm::max(glm::max(t_min_vec.x, t_min_vec.y), t_min_vec.z);
    float t_exit = glm::min(glm::min(t_max_vec.x, t_max_vec.y), t_max_vec.z);

    if (t_enter > t_exit || t_exit < 0.0f) {
        return false; // No intersection or AABB is behind the ray
    }

    if (t_enter < 0.0f) { // Ray origin is inside the AABB
        t_min = 0.0f; // Or a small epsilon to avoid self-intersection
    } else {
        t_min = t_enter;
    }

    // Calculate hit normal
    glm::vec3 hitPoint = rayOrigin + rayDirection * t_min;
    glm::vec3 center = (aabbMin + aabbMax) * 0.5f;
    glm::vec3 dirToCenter = glm::normalize(hitPoint - center);

    // Determine which face was hit
    // This is a simplified approach and might not be perfectly accurate for all cases
    // A more robust approach would compare t_enter with t_min_vec components
    if (t_min == t_min_vec.x) hit_normal = (rayDirection.x < 0) ? glm::vec3(1, 0, 0) : glm::vec3(-1, 0, 0);
    else if (t_min == t_min_vec.y) hit_normal = (rayDirection.y < 0) ? glm::vec3(0, 1, 0) : glm::vec3(0, -1, 0);
    else if (t_min == t_min_vec.z) hit_normal = (rayDirection.z < 0) ? glm::vec3(0, 0, 1) : glm::vec3(0, 0, -1);
    else hit_normal = glm::vec3(0,0,0); // Should not happen if t_min is one of t_min_vec components

    return true;
}


PhysicsSystem::RayCastResult ChunkManager::castRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) {
    PhysicsSystem::RayCastResult result;
    result.hasHit = false;
    // result.bodyID = JPH::BodyID(); // No Jolt BodyID for voxel raycast
    result.hitPosition = glm::vec3(0.0f);
    result.hitNormal = glm::vec3(0.0f);

    float minDistance = std::numeric_limits<float>::max();

    // Voxel traversal parameters
    glm::vec3 currentWorldPos = rayOrigin;
    glm::ivec3 currentVoxel = glm::ivec3(floor(rayOrigin.x / Chunk::VOXEL_SIZE),
                                         floor(rayOrigin.y / Chunk::VOXEL_SIZE),
                                         floor(rayOrigin.z / Chunk::VOXEL_SIZE));

    glm::vec3 step = glm::vec3(glm::sign(rayDirection.x), glm::sign(rayDirection.y), glm::sign(rayDirection.z));
    glm::vec3 tMax; // distance along ray to next voxel boundary
    glm::vec3 tDelta; // distance along ray to cross one voxel

    // Initialize tMax and tDelta
    for (int i = 0; i < 3; ++i) {
        if (rayDirection[i] != 0.0f) {
            if (step[i] > 0) { // Positive direction
                tMax[i] = (static_cast<float>(currentVoxel[i] + 1) * Chunk::VOXEL_SIZE - rayOrigin[i]) / rayDirection[i];
            } else { // Negative direction
                tMax[i] = (static_cast<float>(currentVoxel[i]) * Chunk::VOXEL_SIZE - rayOrigin[i]) / rayDirection[i];
            }
            tDelta[i] = Chunk::VOXEL_SIZE / std::abs(rayDirection[i]);
        } else {
            tMax[i] = std::numeric_limits<float>::max(); // Ray is parallel to this axis
            tDelta[i] = std::numeric_limits<float>::max();
        }
    }

    // Max distance to check (e.g., 100 units)
    float maxRayDistance = 100.0f;
    float currentRayDistance = 0.0f;

    while (currentRayDistance < maxRayDistance) {
        // Get chunk and local voxel coordinates
        glm::vec3 voxelWorldCenter = glm::vec3(currentVoxel.x + 0.5f, currentVoxel.y + 0.5f, currentVoxel.z + 0.5f) * Chunk::VOXEL_SIZE;
        Chunk::ChunkCoord chunkCoord = worldToChunkCoord(voxelWorldCenter);
        Chunk* chunk = getChunk(chunkCoord);

        if (chunk) {
            glm::ivec3 localVoxel = worldToLocalVoxel(voxelWorldCenter, chunkCoord);

            if (chunk->isSolid(localVoxel.x, localVoxel.y, localVoxel.z)) {
                // Found a solid voxel, now do precise intersection with its AABB
                glm::vec3 aabbMin = glm::vec3(currentVoxel) * Chunk::VOXEL_SIZE;
                glm::vec3 aabbMax = aabbMin + glm::vec3(Chunk::VOXEL_SIZE);

                float t_hit;
                glm::vec3 hit_normal;
                if (intersectRayAABB(rayOrigin, rayDirection, aabbMin, aabbMax, t_hit, hit_normal)) {
                    if (t_hit < minDistance) {
                        minDistance = t_hit;
                        result.hasHit = true;
                        result.hitPosition = rayOrigin + rayDirection * t_hit;
                        result.hitNormal = hit_normal;
                        // No Jolt BodyID, so leave it default or set to an invalid value
                        // result.bodyID = JPH::BodyID();
                    }
                }
                // Since we found a hit, we can break early if we only care about the first hit
                // If we need the closest hit among all traversed voxels, we continue
                break; 
            }
        }

        // Advance to the next voxel
        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            currentVoxel.x += static_cast<int>(step.x);
            currentRayDistance = tMax.x;
            tMax.x += tDelta.x;
        } else if (tMax.y < tMax.z) {
            currentVoxel.y += static_cast<int>(step.y);
            currentRayDistance = tMax.y;
            tMax.y += tDelta.y;
        } else {
            currentVoxel.z += static_cast<int>(step.z);
            currentRayDistance = tMax.z;
            tMax.z += tDelta.z;
        }
    }

    return result;
}
