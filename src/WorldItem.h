#pragma once

#include "Sphere.h"
#include "Item.h"
#include <memory>

class WorldItem {
public:
    WorldItem(std::unique_ptr<Item> item, const glm::vec3& position);

    Sphere sphere;
    std::unique_ptr<Item> item;
};
