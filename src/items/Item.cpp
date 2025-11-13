#include "Item.h"

Item::Item(const std::string& name) : name(name) {}

const std::string& Item::getName() const {
    return name;
}
