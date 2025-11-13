#pragma once

#include <array>
#include <glm/glm.hpp>

// A simple struct to hold information needed to create a voxel
struct VoxelInfo {
    glm::vec3 position;
    glm::vec4 color;
};

enum class ObjectType : uint32_t {
    Polygon = 0,
    Text = 1,
    Image = 2,
    Video = 3,
    Brush = 4,
    Cube = 5,
    Voxel = 6,
    Sphere = 7,
    Model = 8,
    Mockup = 9
};

enum class ItemType : uint32_t {
    Apple = 0,
    LaserGun = 1
};

enum class ComponentType : uint32_t {
    Tree = 0,
    House = 1,
    WarTornDome = 2
};
