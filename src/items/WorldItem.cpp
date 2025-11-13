#include "WorldItem.h"

WorldItem::WorldItem(std::unique_ptr<Item> item, const glm::vec3& position)
    : item(std::move(item)), sphere(position, 1.0f) {
}
