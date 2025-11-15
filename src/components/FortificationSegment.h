#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "../types.h"

class FortificationSegment {
public:
    FortificationSegment(const glm::vec3& start,
                         const glm::vec3& end,
                         float height = 10.0f,
                         float thickness = 2.0f,
                         float damageFactor = 0.2f,
                         bool addBattlements = true,
                         int debrisCount = 80,
                        int stoneTextureId = 0,
                       int rubbleTextureId = 0,
                        int crackTextureId = 0);

    std::vector<VoxelInfo> generate() const;

private:
    glm::vec3 start;
    glm::vec3 end;
    float height;
    float thickness;
    float damageFactor;
    bool addBattlements;

    int debrisCount = 80;
    int stoneTextureId = 0;
    int rubbleTextureId = 0;
    int crackTextureId = 0;

    glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t) const;
};
