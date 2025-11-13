#pragma once

#include <vector>
#include <memory>
#include "items/Item.h"

class Inventory {
public:
    void addItem(std::unique_ptr<Item> item);
    void removeItem(Item* item);
    const std::vector<std::unique_ptr<Item>>& getItems() const;

private:
    std::vector<std::unique_ptr<Item>> items;
};
