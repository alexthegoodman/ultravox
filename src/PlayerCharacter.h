#pragma once

#include "Sphere.h"
#include "PhysicsSystem.h"
#include <Jolt/Physics/Character/Character.h>

class PlayerCharacter {
public:
    PlayerCharacter(PhysicsSystem& physicsSystem, const glm::vec3& position);
    ~PlayerCharacter();

    void update(glm::vec3 playerPos);
    void setLinearVelocity(const glm::vec3& velocity);
    glm::vec3 getPosition() const;
    glm::mat4 getModelMatrix() const;

    JPH::Character* character;
    Sphere sphere;

private:
    PhysicsSystem& physicsSystem;
};
