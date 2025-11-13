#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "../types.h"

class House {
public:
    House(const glm::vec3& position, int baseWidth = 6, int baseDepth = 6, int baseHeight = 4);

    std::vector<VoxelInfo> generate() const;

private:
    glm::vec3 basePosition;
    int width;
    int depth;
    int height;
};
