#pragma once

#include <string>

class Item {
public:
    Item(const std::string& name);
    virtual ~Item() = default;

    const std::string& getName() const;

private:
    std::string name;
};
