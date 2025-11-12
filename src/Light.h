#pragma once

#include <glm/glm.hpp>

struct PointLight {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 color;
};
