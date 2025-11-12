#pragma once

#include "FastNoiseLite.h"
#include "Chunk.h"
#include <glm/glm.hpp>

class TerrainGenerator {
public:
    TerrainGenerator();

    void setSeed(int seed);
    void setFrequency(float frequency);
    void setOctaves(int octaves);

    void generateChunk(Chunk* chunk);

private:
    fnl_state noise;
    int seed;
    float frequency;
    int octaves;
};
