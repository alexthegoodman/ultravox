#include "Sphere.h"

Sphere::Sphere(const glm::vec3& position, float radius)
    : position(position), radius(radius) {
}

Sphere::~Sphere() {
}

void Sphere::setPosition(const glm::vec3& position) {
    this->position = position;
}

glm::vec3 Sphere::getPosition() const {
    return position;
}

float Sphere::getRadius() const {
    return radius;
}
