#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "../types.h"

class Tree {
public:
    // Constructor
    Tree(const glm::vec3& position, int recommendedVoxelCount);

    // Generates the voxels for the tree
    std::vector<VoxelInfo> generate() const;

private:
    glm::vec3 basePosition;
    int voxelCount;
};
