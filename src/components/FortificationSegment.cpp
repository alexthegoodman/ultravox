#include "FortificationSegment.h"
#include "../Chunk.h"
#include <cstdlib>
#include <ctime>
#include <cmath>

FortificationSegment::FortificationSegment(
    const glm::vec3& start,
    const glm::vec3& end,
    float height,
    float thickness,
    float damageFactor,
    bool addBattlements,
    int debrisCount,
    int stone,
    int rubble,
    int crack)
    : start(start),
      end(end),
      height(height),
      thickness(thickness),
      damageFactor(damageFactor),
      addBattlements(addBattlements),
      debrisCount(debrisCount),
    stoneTextureId(stone),
    rubbleTextureId(rubble),
    crackTextureId(crack)
{
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned int>(time(0)));
        seeded = true;
    }
}

glm::vec3 FortificationSegment::lerp(const glm::vec3& a, const glm::vec3& b, float t) const {
    return a + t * (b - a);
}

std::vector<VoxelInfo> FortificationSegment::generate() const {
    std::vector<VoxelInfo> voxels;

    const float voxel = Chunk::VOXEL_SIZE;

    glm::vec4 stone(0.55f, 0.55f, 0.58f, 1.0f);
    glm::vec4 rubble(0.45f, 0.42f, 0.40f, 1.0f);
    glm::vec4 crack(0.35f, 0.35f, 0.38f, 1.0f);

    float length = glm::length(end - start);
    int steps = (int)(length / voxel);

    glm::vec3 dir = glm::normalize(end - start);
    glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0,1,0)));

    // --- 1. Wall body ---
    for (int i = 0; i <= steps; ++i) {
        float t = (float)i / steps;
        glm::vec3 midpoint = lerp(start, end, t);

        for (int y = 0; y < (int)height; ++y) {
            for (int w = -(int)thickness; w <= (int)thickness; ++w) {
                glm::vec3 pos = midpoint + right * (float)w * voxel + glm::vec3(0, y * voxel, 0);

                // War damage holes
                float holeChance = damageFactor * (0.5f + ((float)rand() / RAND_MAX));
                if (((float)rand() / RAND_MAX) < holeChance)
                    continue;

                // Color variation
                glm::vec4 color;
                int roll = rand() % 100;
                if (roll < 15)      color = crack;
                else if (roll < 30) color = rubble;
                else                color = stone;

                int textureId = 0;
                int roll = rand() % 100;
                if (roll < 15)      textureId = crackTextureId;
                else if (roll < 30) textureId = rubbleTextureId;
                else                textureId = stoneTextureId;

                voxels.push_back({pos, color, textureId});
            }
        }
    }

    // --- 2. Battlements (crenellations) ---
    if (addBattlements) {
        for (int i = 0; i <= steps; ++i) {
            if (i % 2 == 0) continue; // crenel gap

            float t = (float)i / steps;
            glm::vec3 midpoint = lerp(start, end, t);

            for (int w = -(int)thickness; w <= (int)thickness; ++w) {
                glm::vec3 base = midpoint + right * (float)w * voxel;
                glm::vec3 pos = base + glm::vec3(0, height * voxel, 0);

                if (((float)rand() / RAND_MAX) < damageFactor * 0.3f)
                    continue;

                voxels.push_back({pos, stone, stoneTextureId});
            }
        }
    }

    // --- 3. Debris around base ---
    for (int i = 0; i < debrisCount; ++i) {
        float u = (float)rand() / RAND_MAX;
        glm::vec3 p = lerp(start, end, u);

        float angle = ((float)rand() / RAND_MAX) * 6.28318f;
        float dist = thickness * 4 + ((float)rand() / RAND_MAX) * (thickness * 10);

        glm::vec3 pos = p +
            glm::vec3(cos(angle) * dist, 0, sin(angle) * dist);

        int size = 1 + rand() % 3;
        for (int dx = 0; dx < size; ++dx)
            for (int dy = 0; dy < size; ++dy)
                for (int dz = 0; dz < size; ++dz)
            {
                if (rand() % 100 < 40) continue;
                glm::vec3 ep = pos + glm::vec3(dx, dy, dz) * voxel;
                voxels.push_back({ep, rubble, rubbleTextureId});
            }
    }

    return voxels;
}
