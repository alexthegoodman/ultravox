#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "../types.h"

class WarTornCoolingTower {
public:
    WarTornCoolingTower(const glm::vec3& position,
                        float baseRadius = 10.0f,
                        int height = 32,
                        float damageFactor = 0.25f,
                        int debrisCount = 200,
                        int concreteTextureId = 0, 
                        int scorchedTextureId = 0,
                        int crackedTextureId = 0,
                        int debrisTextureId = 0);

    std::vector<VoxelInfo> generate() const;

private:
    glm::vec3 basePosition;
    float baseRadius;
    int height;
    float damageFactor;
    int debrisCount;

    int concreteTextureId = 0;
    int scorchedTextureId = 0;
    int crackedTextureId = 0;
    int debrisTextureId = 0;

    float radiusAtY(int y) const;
};
