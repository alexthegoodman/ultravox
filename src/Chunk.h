#pragma once

#include "Vertex.h"
#include <vector>
#include <glm/glm.hpp>
#include <cstdint>
#include <fstream>

// Chunk represents a fixed-size grid of voxels
class Chunk {
public:
    static constexpr int CHUNK_SIZE = 32; // 32x32x32 voxels per chunk
    static constexpr float VOXEL_SIZE = 1.0f;
    bool meshDirty;
    
    struct ChunkCoord {
        int x, y, z;
        
        bool operator==(const ChunkCoord& other) const {
            return x == other.x && y == other.y && z == other.z;
        }
        
        bool operator<(const ChunkCoord& other) const {
            if (x != other.x) return x < other.x;
            if (y != other.y) return y < other.y;
            return z < other.z;
        }
    };
    
    // Voxel data (minimal storage)
    struct VoxelData {
        glm::vec4 color;
        uint8_t type; // 0 = air, 1+ = solid types
        
        VoxelData() : color(1.0f), type(0) {}
        VoxelData(const glm::vec4& c, uint8_t t) : color(c), type(t) {}
    };

    // Physics data for a single voxel
    struct PhysicsVoxelData {
        glm::vec3 worldPosition;
        float size; // Assuming uniform size for now
        uint8_t type; // Corresponds to VoxelData::type

        // Default constructor
        PhysicsVoxelData() : worldPosition(0.0f), size(0.0f), type(0) {}

        // Parameterized constructor
        PhysicsVoxelData(const glm::vec3& pos, float s, uint8_t t)
            : worldPosition(pos), size(s), type(t) {}
    };

private:
    ChunkCoord coordinate;
    std::vector<VoxelData> voxels; // Flat array: index = x + y*SIZE + z*SIZE*SIZE
    
    // Cached mesh data
    std::vector<Vertex> vertexCache;
    std::vector<uint32_t> indexCache;
    
    bool isEmpty;

public:
    Chunk(const ChunkCoord& coord) 
        : coordinate(coord), 
          meshDirty(true),
          isEmpty(true) {
        voxels.resize(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);
    }

    void fillVoxels(const VoxelData& voxel) {
        for (int x = 0; x < CHUNK_SIZE; ++x)
            for (int y = 0; y < CHUNK_SIZE; ++y)
                for (int z = 0; z < CHUNK_SIZE; ++z)
                    setVoxel(x, y, z, voxel);
    }
    
    // Get world position of chunk origin
    glm::vec3 getWorldPosition() const {
        return glm::vec3(
            coordinate.x * CHUNK_SIZE * VOXEL_SIZE,
            coordinate.y * CHUNK_SIZE * VOXEL_SIZE,
            coordinate.z * CHUNK_SIZE * VOXEL_SIZE
        );
    }
    
    // Get chunk coordinate
    const ChunkCoord& getCoordinate() const { return coordinate; }
    
    // Set voxel at local position (0 to CHUNK_SIZE-1)
    void setVoxel(int x, int y, int z, const VoxelData& data) {
        if (x < 0 || x >= CHUNK_SIZE || 
            y < 0 || y >= CHUNK_SIZE || 
            z < 0 || z >= CHUNK_SIZE) return;
        
        int index = x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;
        voxels[index] = data;
        meshDirty = true;
        
        if (data.type != 0) isEmpty = false;
    }
    
    // Get voxel at local position
    const VoxelData& getVoxel(int x, int y, int z) const {
        static VoxelData air;
        if (x < 0 || x >= CHUNK_SIZE || 
            y < 0 || y >= CHUNK_SIZE || 
            z < 0 || z >= CHUNK_SIZE) return air;
        
        int index = x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;
        return voxels[index];
    }
    
    // Check if voxel is solid
    bool isSolid(int x, int y, int z) const {
        return getVoxel(x, y, z).type != 0;
    }

    // Check if voxel is a surface voxel (solid and exposed to air)
    bool isSurfaceVoxel(int x, int y, int z) const {
        // If the voxel itself is not solid, it cannot be a surface voxel
        if (!isSolid(x, y, z)) {
            return false;
        }

        // Check 6 neighbors
        // Using getVoxel handles out-of-bounds by returning an air voxel,
        // which is exactly what we need for surface detection.
        if (!isSolid(x + 1, y, z) ||
            !isSolid(x - 1, y, z) ||
            !isSolid(x, y + 1, z) ||
            !isSolid(x, y - 1, z) ||
            !isSolid(x, y, z + 1) ||
            !isSolid(x, y, z - 1)) {
            return true;
        }

        return false; // All neighbors are solid, so it's an interior voxel
    }
    
    
    // Rebuild mesh with greedy meshing
    void rebuildMesh() {
        if (!meshDirty) return;
        
        vertexCache.clear();
        indexCache.clear();
        
        if (isEmpty) {
            meshDirty = false;
            return;
        }
        
        glm::vec3 worldPos = getWorldPosition();
        
        // Simple face culling - only render faces exposed to air
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            for (int y = 0; y < CHUNK_SIZE; ++y) {
                for (int z = 0; z < CHUNK_SIZE; ++z) {
                    if (!isSolid(x, y, z)) continue;
                    
                    const VoxelData& voxel = getVoxel(x, y, z);
                    glm::vec3 pos = worldPos + glm::vec3(x, y, z) * VOXEL_SIZE;
                    
                    // Add debug check:
                    if (std::isnan(pos.x) || std::isnan(pos.y) || std::isnan(pos.z)) {
                        continue; // Skip invalid positions
                    }

                    // Check each face
                    if (!isSolid(x, y + 1, z)) addFace(pos, voxel.color, 0); // Top
                    if (!isSolid(x, y - 1, z)) addFace(pos, voxel.color, 1); // Bottom
                    if (!isSolid(x + 1, y, z)) addFace(pos, voxel.color, 2); // Right
                    if (!isSolid(x - 1, y, z)) addFace(pos, voxel.color, 3); // Left
                    if (!isSolid(x, y, z + 1)) addFace(pos, voxel.color, 4); // Front
                    if (!isSolid(x, y, z - 1)) addFace(pos, voxel.color, 5); // Back

                    // if (!isSolid(x, y + 1, z)) addDoubleSidedFace(pos, voxel.color, 0); // Top
                    // if (!isSolid(x, y - 1, z)) addDoubleSidedFace(pos, voxel.color, 1); // Bottom
                    // if (!isSolid(x + 1, y, z)) addDoubleSidedFace(pos, voxel.color, 2); // Right
                    // if (!isSolid(x - 1, y, z)) addDoubleSidedFace(pos, voxel.color, 3); // Left
                    // if (!isSolid(x, y, z + 1)) addDoubleSidedFace(pos, voxel.color, 4); // Front
                    // if (!isSolid(x, y, z - 1)) addDoubleSidedFace(pos, voxel.color, 5); // Back

                    // addCube(pos, voxel.color);
                }
            }
        }
        
        // meshDirty = false; // Moved to VulkanEngine::updateChunkBuffers
    }
    
    const std::vector<Vertex>& getVertices() const { return vertexCache; }
    const std::vector<uint32_t>& getIndices() const { return indexCache; }
    bool isDirty() const { return meshDirty; }
    bool empty() const { return isEmpty; }
    
    // Binary serialization
    void saveToBinary(std::ofstream& out) const {
        // Write coordinate
        out.write(reinterpret_cast<const char*>(&coordinate), sizeof(ChunkCoord));
        
        // Write isEmpty flag
        out.write(reinterpret_cast<const char*>(&isEmpty), sizeof(bool));
        
        if (isEmpty) return; // Skip empty chunks
        
        // Write voxel data
        out.write(reinterpret_cast<const char*>(voxels.data()), 
                  voxels.size() * sizeof(VoxelData));
    }
    
    bool loadFromBinary(std::ifstream& in) {
        // Read coordinate
        in.read(reinterpret_cast<char*>(&coordinate), sizeof(ChunkCoord));
        
        // Read isEmpty flag
        in.read(reinterpret_cast<char*>(&isEmpty), sizeof(bool));
        
        if (isEmpty) {
            voxels.clear();
            voxels.resize(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);
            return true;
        }
        
        // Read voxel data
        voxels.resize(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);
        in.read(reinterpret_cast<char*>(voxels.data()), 
                voxels.size() * sizeof(VoxelData));
        
        meshDirty = true;
        return in.good();
    }

private:
    void addCube(const glm::vec3& pos, const glm::vec4& color) {
        float w = VOXEL_SIZE;
        float h = VOXEL_SIZE;
        float d = VOXEL_SIZE;
        float hw = w / 2.0f; // half width
        float hh = h / 2.0f; // half height
        float hd = d / 2.0f; // half depth

        // Define the 8 cube corners (local positions)
        glm::vec3 localPositions[8] = {
            glm::vec3(-hw, -hh,  hd), // 0 Front-bottom-left
            glm::vec3( hw, -hh,  hd), // 1 Front-bottom-right
            glm::vec3( hw,  hh,  hd), // 2 Front-top-right
            glm::vec3(-hw,  hh,  hd), // 3 Front-top-left
            glm::vec3(-hw, -hh, -hd), // 4 Back-bottom-left
            glm::vec3( hw, -hh, -hd), // 5 Back-bottom-right
            glm::vec3( hw,  hh, -hd), // 6 Back-top-right
            glm::vec3(-hw,  hh, -hd)  // 7 Back-top-left
        };

        // Transform to world position
        glm::vec3 positions[8];
        for (int i = 0; i < 8; ++i) {
            positions[i] = pos + localPositions[i];
        }

        // Define faces with indices and normals
        struct Face {
            int indices[6];
            glm::vec3 normal;
        };

        Face faces[6] = {
            {{0, 1, 2, 0, 2, 3}, glm::vec3( 0,  0,  1)}, // Front
            {{5, 4, 7, 5, 7, 6}, glm::vec3( 0,  0, -1)}, // Back
            {{3, 2, 6, 3, 6, 7}, glm::vec3( 0,  1,  0)}, // Top
            {{4, 5, 1, 4, 1, 0}, glm::vec3( 0, -1,  0)}, // Bottom
            {{1, 5, 6, 1, 6, 2}, glm::vec3( 1,  0,  0)}, // Right
            {{4, 0, 3, 4, 3, 7}, glm::vec3(-1,  0,  0)}  // Left
        };

        // Build geometry for all faces
        for (int faceIdx = 0; faceIdx < 6; ++faceIdx) {
            const Face& face = faces[faceIdx];
            uint32_t startIndex = static_cast<uint32_t>(vertexCache.size());

            // Add 6 vertices (2 triangles per face)
            for (int i = 0; i < 6; ++i) {
                int idx = face.indices[i];
                glm::vec3 p = positions[idx];
                
                Vertex v;
                v.position = p;
                v.texCoords = glm::vec2(0.0f, 0.0f);
                v.color = color;
                v.gradientCoords = glm::vec2((p.x - pos.x + hw) / w, (p.y - pos.y + hh) / h);
                v.objectType = 6.0f; // 6 = solid voxel
                v.normal = face.normal;
                vertexCache.push_back(v);
            }

            // Add indices (already in the correct order from face.indices)
            indexCache.push_back(startIndex + 0);
            indexCache.push_back(startIndex + 1);
            indexCache.push_back(startIndex + 2);
            indexCache.push_back(startIndex + 3);
            indexCache.push_back(startIndex + 4);
            indexCache.push_back(startIndex + 5);
        }
    }

    // void addFace(const glm::vec3& pos, const glm::vec4& color, int faceIndex) {
    //     uint32_t baseIndex = static_cast<uint32_t>(vertexCache.size());
        
    //     // Define face vertices based on direction
    //     glm::vec3 vertices[4];
    //     glm::vec3 normal;
        
    //     switch (faceIndex) {
    //         case 0: // Top (+Y)
    //             vertices[0] = pos + glm::vec3(0, VOXEL_SIZE, 0);
    //             vertices[1] = pos + glm::vec3(VOXEL_SIZE, VOXEL_SIZE, 0);
    //             vertices[2] = pos + glm::vec3(VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE);
    //             vertices[3] = pos + glm::vec3(0, VOXEL_SIZE, VOXEL_SIZE);
    //             normal = glm::vec3(0, 1, 0);
    //             break;
    //         case 1: // Bottom (-Y)
    //             vertices[0] = pos + glm::vec3(0, 0, 0);
    //             vertices[1] = pos + glm::vec3(0, 0, VOXEL_SIZE);
    //             vertices[2] = pos + glm::vec3(VOXEL_SIZE, 0, VOXEL_SIZE);
    //             vertices[3] = pos + glm::vec3(VOXEL_SIZE, 0, 0);
    //             normal = glm::vec3(0, -1, 0);
    //             break;
    //         case 2: // Right (+X)
    //             vertices[0] = pos + glm::vec3(VOXEL_SIZE, 0, 0);
    //             vertices[1] = pos + glm::vec3(VOXEL_SIZE, 0, VOXEL_SIZE);
    //             vertices[2] = pos + glm::vec3(VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE);
    //             vertices[3] = pos + glm::vec3(VOXEL_SIZE, VOXEL_SIZE, 0);
    //             normal = glm::vec3(1, 0, 0);
    //             break;
    //         case 3: // Left (-X)
    //             vertices[0] = pos + glm::vec3(0, 0, 0);
    //             vertices[1] = pos + glm::vec3(0, VOXEL_SIZE, 0);
    //             vertices[2] = pos + glm::vec3(0, VOXEL_SIZE, VOXEL_SIZE);
    //             vertices[3] = pos + glm::vec3(0, 0, VOXEL_SIZE);
    //             normal = glm::vec3(-1, 0, 0);
    //             break;
    //         case 4: // Front (+Z)
    //             vertices[0] = pos + glm::vec3(0, 0, VOXEL_SIZE);
    //             vertices[1] = pos + glm::vec3(0, VOXEL_SIZE, VOXEL_SIZE);
    //             vertices[2] = pos + glm::vec3(VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE);
    //             vertices[3] = pos + glm::vec3(VOXEL_SIZE, 0, VOXEL_SIZE);
    //             normal = glm::vec3(0, 0, 1);
    //             break;
    //         case 5: // Back (-Z)
    //             vertices[0] = pos + glm::vec3(0, 0, 0);
    //             vertices[1] = pos + glm::vec3(VOXEL_SIZE, 0, 0);
    //             vertices[2] = pos + glm::vec3(VOXEL_SIZE, VOXEL_SIZE, 0);
    //             vertices[3] = pos + glm::vec3(0, VOXEL_SIZE, 0);
    //             normal = glm::vec3(0, 0, -1);
    //             break;
    //     }

    //     for (int i = 0; i < 4; ++i) {
    //         Vertex v;
    //         v.position = vertices[i];
    //         v.texCoords = glm::vec2(0.0f);      // Initialize (or use proper UVs)
    //         v.color = color;
    //         v.gradientCoords = glm::vec2(0.0f); // Initialize
    //         v.objectType = 6.0f;                // 6 = solid voxel
    //         v.normal = normal;
    //         vertexCache.push_back(v);
    //     }
        
    //     // Add indices (two triangles per face)
    //     indexCache.push_back(baseIndex + 0);
    //     indexCache.push_back(baseIndex + 1);
    //     indexCache.push_back(baseIndex + 2);
        
    //     indexCache.push_back(baseIndex + 0);
    //     indexCache.push_back(baseIndex + 2);
    //     indexCache.push_back(baseIndex + 3);
    // }

    void addFace(const glm::vec3& pos, const glm::vec4& color, int faceIndex) {
        uint32_t baseIndex = static_cast<uint32_t>(vertexCache.size());
        
        // Define face vertices based on direction
        // Vertices are ordered counter-clockwise when viewed from outside
        glm::vec3 vertices[4];
        glm::vec3 normal;
        
        switch (faceIndex) {
            case 0: // Top (+Y) - looking down at it
                vertices[0] = pos + glm::vec3(0, VOXEL_SIZE, 0);
                vertices[1] = pos + glm::vec3(0, VOXEL_SIZE, VOXEL_SIZE);
                vertices[2] = pos + glm::vec3(VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE);
                vertices[3] = pos + glm::vec3(VOXEL_SIZE, VOXEL_SIZE, 0);
                normal = glm::vec3(0, 1, 0);
                break;
            case 1: // Bottom (-Y) - looking up at it
                vertices[0] = pos + glm::vec3(0, 0, 0);
                vertices[1] = pos + glm::vec3(VOXEL_SIZE, 0, 0);
                vertices[2] = pos + glm::vec3(VOXEL_SIZE, 0, VOXEL_SIZE);
                vertices[3] = pos + glm::vec3(0, 0, VOXEL_SIZE);
                normal = glm::vec3(0, -1, 0);
                break;
            case 2: // Right (+X) - looking from the right
                vertices[0] = pos + glm::vec3(VOXEL_SIZE, 0, 0);
                vertices[1] = pos + glm::vec3(VOXEL_SIZE, VOXEL_SIZE, 0);
                vertices[2] = pos + glm::vec3(VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE);
                vertices[3] = pos + glm::vec3(VOXEL_SIZE, 0, VOXEL_SIZE);
                normal = glm::vec3(1, 0, 0);
                break;
            case 3: // Left (-X) - looking from the left
                vertices[0] = pos + glm::vec3(0, 0, 0);
                vertices[1] = pos + glm::vec3(0, 0, VOXEL_SIZE);
                vertices[2] = pos + glm::vec3(0, VOXEL_SIZE, VOXEL_SIZE);
                vertices[3] = pos + glm::vec3(0, VOXEL_SIZE, 0);
                normal = glm::vec3(-1, 0, 0);
                break;
            case 4: // Front (+Z) - looking from the front
                vertices[0] = pos + glm::vec3(0, 0, VOXEL_SIZE);
                vertices[1] = pos + glm::vec3(VOXEL_SIZE, 0, VOXEL_SIZE);
                vertices[2] = pos + glm::vec3(VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE);
                vertices[3] = pos + glm::vec3(0, VOXEL_SIZE, VOXEL_SIZE);
                normal = glm::vec3(0, 0, 1);
                break;
            case 5: // Back (-Z) - looking from the back
                vertices[0] = pos + glm::vec3(0, 0, 0);
                vertices[1] = pos + glm::vec3(0, VOXEL_SIZE, 0);
                vertices[2] = pos + glm::vec3(VOXEL_SIZE, VOXEL_SIZE, 0);
                vertices[3] = pos + glm::vec3(VOXEL_SIZE, 0, 0);
                normal = glm::vec3(0, 0, -1);
                break;
        }

        // Add vertices
        for (int i = 0; i < 4; ++i) {
            Vertex v;
            v.position = vertices[i];
            v.texCoords = glm::vec2(0.0f);
            v.color = color;
            v.gradientCoords = glm::vec2(0.0f);
            v.objectType = 6.0f;
            v.normal = normal;
            vertexCache.push_back(v);
        }
        
        // Add indices (counter-clockwise winding)
        indexCache.push_back(baseIndex + 0);
        indexCache.push_back(baseIndex + 1);
        indexCache.push_back(baseIndex + 2);
        
        indexCache.push_back(baseIndex + 0);
        indexCache.push_back(baseIndex + 2);
        indexCache.push_back(baseIndex + 3);
    }

    // Alternative: If you truly need double-sided faces
    void addDoubleSidedFace(const glm::vec3& pos, const glm::vec4& color, int faceIndex) {
        // Add the normal face
        addFace(pos, color, faceIndex);
        
        // Add the reverse face by duplicating vertices with flipped normal
        uint32_t baseIndex = static_cast<uint32_t>(vertexCache.size());
        
        // Copy the last 4 vertices we just added
        for (int i = 0; i < 4; ++i) {
            Vertex v = vertexCache[vertexCache.size() - 4 + i];
            v.normal = -v.normal; // Flip the normal
            vertexCache.push_back(v);
        }
        
        // Add indices with reversed winding order
        indexCache.push_back(baseIndex + 0);
        indexCache.push_back(baseIndex + 2);
        indexCache.push_back(baseIndex + 1);
        
        indexCache.push_back(baseIndex + 0);
        indexCache.push_back(baseIndex + 3);
        indexCache.push_back(baseIndex + 2);
    }
};