#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "../types.h"

class WarTornDome {
public:
    WarTornDome(const glm::vec3& position, float radius = 6.0f, float damageFactor = 0.25f, int debrisCount = 50);

    std::vector<VoxelInfo> generate() const;

private:
    glm::vec3 basePosition;
    float radius;
    float damageFactor; // 0.0 = pristine, 1.0 = almost destroyed
    int debrisCount;
};
