#include "WarTornDome.h"
#include "../Chunk.h"
#include <cstdlib>
#include <ctime>
#include <cmath>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

WarTornDome::WarTornDome(const glm::vec3& position, float radius, float damageFactor, int debrisCount, int domeTextureId, int debrisTextureId)
    : basePosition(position), radius(radius), damageFactor(damageFactor), debrisCount(debrisCount), domeTextureId(domeTextureId), debrisTextureId(debrisTextureId) {
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned int>(time(0)));
        seeded = true;
    }
}

std::vector<VoxelInfo> WarTornDome::generate() const {
    std::vector<VoxelInfo> voxels;

    // Base colors
    glm::vec4 metalColor(0.5f, 0.5f, 0.55f, 1.0f);
    glm::vec4 scorchedColor(0.2f, 0.2f, 0.2f, 1.0f);
    glm::vec4 debrisColor(0.3f, 0.25f, 0.25f, 1.0f);

    // Approximate voxel radius
    const float voxel = Chunk::VOXEL_SIZE;

    int r = static_cast<int>(radius);
    float shellThickness = glm::clamp(radius * 0.15f, 1.0f, 3.0f);

    // Generate damaged dome shell
    for (int x = -r; x <= r; ++x) {
        for (int y = 0; y <= r; ++y) { // dome (half-sphere)
            for (int z = -r; z <= r; ++z) {
                float dist = sqrtf(float(x * x + y * y + z * z));

                // Only near the surface of the sphere
                if (dist < radius - shellThickness || dist > radius)
                    continue;

                // Damage: probabilistically remove voxels
                float holeChance = damageFactor * (0.5f + static_cast<float>(rand()) / RAND_MAX);
                if (static_cast<float>(rand()) / RAND_MAX < holeChance)
                    continue;

                // Add some visual scorch variation
                glm::vec4 color = (rand() % 100 < 15) ? scorchedColor : metalColor;

                glm::vec3 voxelPos = basePosition + glm::vec3(x, y, z) * voxel;
                voxels.push_back({voxelPos, color, domeTextureId});
            }
        }
    }

    // Scatter debris fragments
    for (int i = 0; i < debrisCount; ++i) {
        float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * M_PI;
        float dist = radius + static_cast<float>(rand()) / RAND_MAX * (radius * 0.8f);
        float height = static_cast<float>(rand()) / RAND_MAX * (radius * 0.3f);

        glm::vec3 debrisPos = basePosition +
            glm::vec3(cos(angle) * dist, height, sin(angle) * dist);

        // Each debris piece: 1–3 voxels
        int size = 1 + rand() % 3;
        for (int dx = 0; dx < size; ++dx) {
            for (int dy = 0; dy < size; ++dy) {
                for (int dz = 0; dz < size; ++dz) {
                    glm::vec3 voxelPos = debrisPos + glm::vec3(dx, dy, dz) * voxel;
                    voxels.push_back({voxelPos, debrisColor, debrisTextureId});
                }
            }
        }
    }

    return voxels;
}
