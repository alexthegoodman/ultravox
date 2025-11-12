#pragma once

#include <vector>
#include <glm/glm.hpp>

// A simple struct to hold information needed to create a voxel
struct VoxelInfo {
    glm::vec3 position;
    glm::vec4 color;
};

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
