#include "TerrainGenerator.h"

#define FNL_IMPL
#include "FastNoiseLite.h"

TerrainGenerator::TerrainGenerator(int grassTextureId, int dirtTextureId, int stoneTextureId) 
    : seed(1337), frequency(0.02f), octaves(4), 
      grassTextureId(grassTextureId), dirtTextureId(dirtTextureId), stoneTextureId(stoneTextureId) {
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
            int height = static_cast<int>((noiseVal + 1.0f) * 0.5f * 8);

            for (int y = 0; y < height; ++y) {
                int textureToUse = 0; // Default texture ID
                if (y < height - 5) {
                    // Stone
                    textureToUse = stoneTextureId;
                } else if (y < height - 1) {
                    // Dirt
                    textureToUse = dirtTextureId;
                } else {
                    // Grass
                    textureToUse = grassTextureId;
                }
                chunk->setVoxel(x, y, z, Chunk::VoxelData(glm::vec4(1.0f), 1, textureToUse));

                glm::vec3 voxelWorldPos = chunkWorldPos + glm::vec3(x, y, z) * Chunk::VOXEL_SIZE;
                Chunk::PhysicsVoxelData physicsData(voxelWorldPos, Chunk::VOXEL_SIZE, 1);
            }
        }
    }
}
