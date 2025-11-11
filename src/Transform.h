#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform {
public:
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    Transform()
        : position(0.0f), rotation(1.0f, 0.0f, 0.0f, 0.0f), scale(1.0f) {}

    glm::mat4 getModelMatrix() const {
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 rot = glm::mat4_cast(rotation);
        glm::mat4 sc = glm::scale(glm::mat4(1.0f), scale);
        return trans * rot * sc;
    }
};
