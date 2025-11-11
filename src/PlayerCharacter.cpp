#include "PlayerCharacter.h"
#include "Logger.h"

PlayerCharacter::PlayerCharacter(PhysicsSystem& physicsSystem, const glm::vec3& position)
    : physicsSystem(physicsSystem), sphere(position, 0.5f) {
    LOG("Creating PlayerCharacter");
    character = physicsSystem.createCharacter(position);
}

PlayerCharacter::~PlayerCharacter() {
    LOG("Destroying PlayerCharacter");
    physicsSystem.destroyCharacter(character);
}

void PlayerCharacter::update(glm::vec3 playerPos) {
    if (character) {
        sphere.setPosition(playerPos);
    } else {
        LOG("NO CHARACTER 1");
    }
}

void PlayerCharacter::setLinearVelocity(const glm::vec3& velocity) {
    if (character) {
        character->SetLinearVelocity(JPH::Vec3(velocity.x, velocity.y, velocity.z));
    } else {
        LOG("NO CHARACTER 2");
    }
}

glm::vec3 PlayerCharacter::getPosition() const {
    if (character) {
        JPH::RVec3 pos = character->GetPosition();
        return glm::vec3(float(pos.GetX()), float(pos.GetY()), float(pos.GetZ()));
    } else {
        LOG("NO CHARACTER POSITION");
    }
    return glm::vec3(0.0f);
}

glm::mat4 PlayerCharacter::getModelMatrix() const {
    return sphere.transform.getModelMatrix();
}
