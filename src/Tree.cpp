#include "Tree.h"
#include "Chunk.h" // For VOXEL_SIZE
#include <cstdlib> // For rand()
#include <ctime>   // For time()

Tree::Tree(const glm::vec3& position, int recommendedVoxelCount)
    : basePosition(position), voxelCount(recommendedVoxelCount) {
    // Seed the random number generator once
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned int>(time(0)));
        seeded = true;
    }
}

std::vector<VoxelInfo> Tree::generate() const {
    std::vector<VoxelInfo> voxels;

    // Basic parameters for the tree
    int trunkHeight = 5 + (voxelCount / 20);
    int canopyRadius = 2 + (voxelCount / 40);

    // Trunk color
    glm::vec4 trunkColor(0.5f, 0.35f, 0.05f, 1.0f);

    // Generate trunk
    for (int i = 0; i < trunkHeight; ++i) {
        glm::vec3 voxelPos = basePosition + glm::vec3(0, i * Chunk::VOXEL_SIZE, 0);
        voxels.push_back({voxelPos, trunkColor});
    }

    // Generate leaves (canopy)
    glm::vec3 canopyCenter = basePosition + glm::vec3(0, trunkHeight * Chunk::VOXEL_SIZE, 0);

    for (int x = -canopyRadius; x <= canopyRadius; ++x) {
        for (int y = -canopyRadius; y <= canopyRadius; ++y) {
            for (int z = -canopyRadius; z <= canopyRadius; ++z) {
                // Create a somewhat spherical shape
                if (x*x + y*y + z*z > canopyRadius * canopyRadius) {
                    continue;
                }

                // Leaf color variation
                float r = 0.0f;
                float g = 0.4f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.4f)); // 0.4 to 0.8
                float b = 0.0f;
                glm::vec4 leafColor(r, g, b, 1.0f);

                glm::vec3 voxelPos = canopyCenter + glm::vec3(x, y, z) * Chunk::VOXEL_SIZE;
                voxels.push_back({voxelPos, leafColor});
            }
        }
    }

    return voxels;
}
