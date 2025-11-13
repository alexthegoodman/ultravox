#include "House.h"
#include "../Chunk.h"
#include <cstdlib>
#include <ctime>

House::House(const glm::vec3& position, int baseWidth, int baseDepth, int baseHeight)
    : basePosition(position), width(baseWidth), depth(baseDepth), height(baseHeight) {
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned int>(time(0)));
        seeded = true;
    }
}

std::vector<VoxelInfo> House::generate() const {
    std::vector<VoxelInfo> voxels;

    // Slight random variation
    int w = width + (rand() % 3 - 1);   // ±1 variation
    int d = depth + (rand() % 3 - 1);
    int h = height + (rand() % 2);      // ±0–1 height variance

    // Base colors
    glm::vec4 wallColor(0.7f + ((rand() % 30) / 100.0f), 0.7f, 0.6f, 1.0f);
    glm::vec4 roofColor(0.4f, 0.1f, 0.05f, 1.0f);
    glm::vec4 doorColor(0.3f, 0.2f, 0.1f, 1.0f);
    glm::vec4 windowColor(0.3f, 0.5f, 0.8f, 1.0f);

    // Construct house walls
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            for (int z = 0; z < d; ++z) {
                // Only draw outer shell
                bool isEdge = (x == 0 || x == w - 1 || z == 0 || z == d - 1);
                if (!isEdge) continue;

                // Door on front center (z == 0)
                bool isDoor = (z == 0 && y < 2 && x == w / 2);

                // Window holes
                bool isWindow = (!isDoor && y == 2 && (x == 1 || x == w - 2) && (z == 0 || z == d - 1));

                if (isDoor) {
                    glm::vec3 voxelPos = basePosition + glm::vec3(x, y, z) * Chunk::VOXEL_SIZE;
                    voxels.push_back({voxelPos, doorColor});
                } else if (!isWindow) {
                    glm::vec3 voxelPos = basePosition + glm::vec3(x, y, z) * Chunk::VOXEL_SIZE;
                    voxels.push_back({voxelPos, wallColor});
                }
            }
        }
    }

    // Add simple flat or sloped roof
    bool sloped = (rand() % 2 == 0);
    int roofHeight = sloped ? 2 : 1;

    for (int y = 0; y < roofHeight; ++y) {
        for (int x = -y; x < w + y; ++x) {
            for (int z = -y; z < d + y; ++z) {
                // Skip inside of roof for performance
                if (x < 0 || z < 0 || x >= w || z >= d) continue;

                glm::vec3 voxelPos = basePosition + glm::vec3(x, h + y, z) * Chunk::VOXEL_SIZE;
                voxels.push_back({voxelPos, roofColor});
            }
        }
    }

    // Optionally add windows as transparent placeholders
    for (int x = 1; x < w - 1; ++x) {
        for (int z = 1; z < d - 1; ++z) {
            if (rand() % 10 < 2) { // ~20% chance
                glm::vec3 voxelPos = basePosition + glm::vec3(x, 2, z) * Chunk::VOXEL_SIZE;
                voxels.push_back({voxelPos, windowColor});
            }
        }
    }

    return voxels;
}
