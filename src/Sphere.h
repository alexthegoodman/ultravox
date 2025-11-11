#pragma once

#include "Transform.h"

class Sphere {
public:
    Transform transform;

    // Sphere() = default;
    Sphere(const glm::vec3& position, float radius) {
        transform.position = position;
        // transform.scale = glm::vec3(radius); // makes no sense
    }

    void setPosition(const glm::vec3& position) {
        transform.position = position;
    }

    glm::vec3 getPosition() const {
        return transform.position;
    }
};
