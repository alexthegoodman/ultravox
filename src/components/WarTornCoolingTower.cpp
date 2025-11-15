#include "WarTornCoolingTower.h"
#include "Chunk.h"
#include <cstdlib>
#include <ctime>
#include <cmath>

WarTornCoolingTower::WarTornCoolingTower(
    const glm::vec3& position,
    float baseRadius,
    int height,
    float damageFactor,
    int debrisCount,
    int concrete,
    int scorched,
    int cracked,
    int debris)
    : basePosition(position),
      baseRadius(baseRadius),
      height(height),
      damageFactor(damageFactor),
      debrisCount(debrisCount),
        concreteTextureId(concrete), 
        scorchedTextureId(scorched),
        crackedTextureId(cracked),
        debrisTextureId(debris)
{
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned int>(time(0)));
        seeded = true;
    }
}

// --- Hyperboloid Profile (real cooling tower shape) ---
float WarTornCoolingTower::radiusAtY(int y) const {
    float t = (float)y / (float)height; // 0 → 1

    float bottom = baseRadius;
    float neck   = baseRadius * 0.55f;
    float top    = baseRadius * 0.85f;

    if (t < 0.5f) {
        // bottom → neck
        return bottom + (neck - bottom) * (t / 0.5f);
    } else {
        // neck → top
        return neck + (top - neck) * ((t - 0.5f) / 0.5f);
    }
}

std::vector<VoxelInfo> WarTornCoolingTower::generate() const {
    std::vector<VoxelInfo> voxels;

    const float voxel = Chunk::VOXEL_SIZE;

    // Color palette
    glm::vec4 concrete(0.65f, 0.65f, 0.68f, 1.0f);
    glm::vec4 scorched(0.25f, 0.25f, 0.28f, 1.0f);
    glm::vec4 cracked(0.55f, 0.55f, 0.60f, 1.0f);
    glm::vec4 debrisColor(0.4f, 0.38f, 0.35f, 1.0f);

    // ----- 1. Outer hyperbolic shell -----
    for (int y = 0; y < height; ++y) {
        float localR = radiusAtY(y);

        for (int x = -(int)localR - 1; x <= (int)localR + 1; ++x) {
            for (int z = -(int)localR - 1; z <= (int)localR + 1; ++z) {

                float dist = sqrtf(x*x + z*z);

                // Shell thickness of 2 voxels
                if (dist > localR || dist < localR - 2.0f)
                    continue;

                // War damage – skip voxels to create holes
                float holeChance = damageFactor * (0.5f + ((float)rand() / RAND_MAX));
                if (((float)rand() / RAND_MAX) < holeChance)
                    continue;

                // Random concrete variation
                glm::vec4 color;
                int roll = rand() % 100;
                if (roll < 10)      color = scorched;
                else if (roll < 25) color = cracked;
                else                color = concrete;

                glm::vec3 pos = basePosition + glm::vec3(x, y, z) * voxel;
                voxels.push_back({pos, color});
            }
        }
    }

    // ----- 2. Top rim / lip -----
    int lipY = height - 1;
    float lipR = radiusAtY(lipY) + 1.2f;

    for (int x = -(int)lipR - 1; x <= (int)lipR + 1; ++x) {
        for (int z = -(int)lipR - 1; z <= (int)lipR + 1; ++z) {
            float dist = sqrtf(x*x + z*z);
            if (dist > lipR || dist < lipR - 1.5f) continue;

            // Damage still applies
            if (((float)rand() / RAND_MAX) < damageFactor * 0.4f)
                continue;

            glm::vec3 pos = basePosition + glm::vec3(x, lipY, z) * voxel;
            voxels.push_back({pos, concrete});
        }
    }

    // ----- 3. Interior upper gap (steam exit) -----
    // int steamY = height - 2;
    // float steamR = radiusAtY(steamY) * 0.6f;

    // for (int x = -(int)steamR; x <= (int)steamR; ++x) {
    //     for (int z = -(int)steamR; z <= (int)steamR; ++z) {
    //         float dist = sqrtf(x*x + z*z);
    //         if (dist > steamR) continue;

    //         // This intentionally empties interior
    //         // No voxels added — hollow core
    //     }
    // }

    // ----- 4. Debris field -----

    for (int i = 0; i < debrisCount; ++i) {
        float ang = ((float)rand() / RAND_MAX) * 6.28318f;
        float dist = baseRadius + 3.0f + ((float)rand() / RAND_MAX) * (baseRadius * 2.0f);
        float h = ((float)rand() / RAND_MAX) * 2.0f;

        glm::vec3 base = basePosition +
            glm::vec3(cosf(ang) * dist, h, sinf(ang) * dist);

        int s = 1 + rand() % 3; // debris chunk size
        for (int dx = 0; dx < s; ++dx)
            for (int dy = 0; dy < s; ++dy)
                for (int dz = 0; dz < s; ++dz) {

                    if (rand() % 100 < 30) continue;

                    glm::vec3 pos = base + glm::vec3(dx, dy, dz) * voxel;
                    voxels.push_back({pos, debrisColor});
                }
    }

    return voxels;
}
