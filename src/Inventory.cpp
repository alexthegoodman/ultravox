#include "Inventory.h"
#include <algorithm>

void Inventory::addItem(std::unique_ptr<Item> item) {
    items.push_back(std::move(item));
}

void Inventory::removeItem(Item* item) {
    items.erase(std::remove_if(items.begin(), items.end(),
        [item](const std::unique_ptr<Item>& p) { return p.get() == item; }), items.end());
}

const std::vector<std::unique_ptr<Item>>& Inventory::getItems() const {
    return items;
}
