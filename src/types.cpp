#include <array>
#include <glm/glm.hpp> // Include for glm::vec3

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

// Custom operator< for glm::vec3 to allow its use in std::map
namespace glm {
    bool operator<(const vec3& a, const vec3& b) {
        if (a.x != b.x) return a.x < b.x;
        if (a.y != b.y) return a.y < b.y;
        return a.z < b.z;
    }
}