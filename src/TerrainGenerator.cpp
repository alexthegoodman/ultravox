#include "TerrainGenerator.h"
#include "helpers.h" // For HSLtoRGB

#define FNL_IMPL
#include "FastNoiseLite.h"

TerrainGenerator::TerrainGenerator() : seed(1337), frequency(0.02f), octaves(4) {
    noise = fnlCreateState();
    noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    noise.fractal_type = FNL_FRACTAL_FBM;
    setSeed(seed);
    setFrequency(frequency);
    setOctaves(octaves);
}

void TerrainGenerator::setSeed(int newSeed) {
    seed = newSeed;
    noise.seed = seed;
}

void TerrainGenerator::setFrequency(float newFrequency) {
    frequency = newFrequency;
    noise.frequency = frequency;
}

void TerrainGenerator::setOctaves(int newOctaves) {
    octaves = newOctaves;
    noise.octaves = octaves;
}

void TerrainGenerator::generateChunk(Chunk* chunk) {
    const Chunk::ChunkCoord& coord = chunk->getCoordinate();
    glm::vec3 chunkWorldPos = chunk->getWorldPosition();

    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
            float worldX = chunkWorldPos.x + x * Chunk::VOXEL_SIZE;
            float worldZ = chunkWorldPos.z + z * Chunk::VOXEL_SIZE;

            float noiseVal = fnlGetNoise2D(&noise, worldX, worldZ);

            // Map noise from [-1, 1] to a height in voxels, e.g., [0, 64]
            int height = static_cast<int>((noiseVal + 1.0f) * 0.5f * 64);

            for (int y = 0; y < height; ++y) {
                glm::vec4 color;
                if (y < height - 5) {
                    // Stone
                    color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
                } else if (y < height - 1) {
                    // Dirt
                    color = glm::vec4(0.6f, 0.4f, 0.2f, 1.0f);
                } else {
                    // Grass
                    color = glm::vec4(0.2f, 0.8f, 0.2f, 1.0f);
                }
                chunk->setVoxel(x, y, z, Chunk::VoxelData(color, 1));

                // Insert physics voxel data into the octree
                glm::vec3 voxelWorldPos = chunkWorldPos + glm::vec3(x, y, z) * Chunk::VOXEL_SIZE;
                Chunk::PhysicsVoxelData physicsData(voxelWorldPos, Chunk::VOXEL_SIZE, 1);
                // This part needs to be handled in ChunkManager, which has the octree
                // physicsOctree.insert(Vector3(voxelWorldPos.x, voxelWorldPos.y, voxelWorldPos.z), physicsData);
            }
        }
    }
}
