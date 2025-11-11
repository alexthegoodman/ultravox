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

void PlayerCharacter::update() {
    if (character) {
        sphere.setPosition(getPosition());
    }
}

void PlayerCharacter::setLinearVelocity(const glm::vec3& velocity) {
    if (character) {
        character->SetLinearVelocity(JPH::Vec3(velocity.x, velocity.y, velocity.z));
    }
}

glm::vec3 PlayerCharacter::getPosition() const {
    if (character) {
        JPH::RVec3 pos = character->GetPosition();
        return glm::vec3(float(pos.GetX()), float(pos.GetY()), float(pos.GetZ()));
    }
    return glm::vec3(0.0f);
}

glm::mat4 PlayerCharacter::getModelMatrix() const {
    return sphere.transform.getModelMatrix();
}
