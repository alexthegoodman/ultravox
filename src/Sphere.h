#pragma once

#include <glm/glm.hpp>

class Sphere {
public:
    Sphere(const glm::vec3& position, float radius);
    ~Sphere();

    void setPosition(const glm::vec3& position);
    glm::vec3 getPosition() const;
    float getRadius() const;

private:
    glm::vec3 position;
    float radius;
};
