#pragma once

#include "Sphere.h"
#include "PhysicsSystem.h"
#include <Jolt/Physics/Character/Character.h>

class PlayerCharacter {
public:
    PlayerCharacter(PhysicsSystem& physicsSystem, const glm::vec3& position);
    ~PlayerCharacter();

    void setLinearVelocity(const glm::vec3& velocity);
    glm::vec3 getPosition() const;

private:
    PhysicsSystem& physicsSystem;
    JPH::Character* character;
    Sphere sphere;
};
