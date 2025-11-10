#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>

// --------------------
// Basic vector & bounds
// --------------------

struct Vector3 {
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

struct BoundingBox {
    Vector3 min;
    Vector3 max;
};

// --------------------
// Node data wrapper
// --------------------

template <typename T>
struct OctreeData {
    Vector3 position;
    T data;
};

// --------------------
// Node class
// --------------------

template <typename T>
class OctreeNode {
public:
    BoundingBox bounds;
    std::vector<std::unique_ptr<OctreeNode<T>>> children;
    std::vector<OctreeData<T>> items;
    int depth;

    explicit OctreeNode(const BoundingBox& bounds_, int depth_ = 0)
        : bounds(bounds_), depth(depth_) {}

    bool isLeaf() const { return children.empty(); }
};

// --------------------
// Octree statistics
// --------------------

struct OctreeStats {
    size_t totalNodes = 0;
    size_t leafNodes = 0;
    size_t totalItems = 0;
    int maxDepth = 0;
    float averageItemsPerLeaf = 0.f;
};

// --------------------
// Main Octree class
// --------------------

template <typename T>
class Octree {
public:
    // Default constructor with a large default bounding box
    explicit Octree()
        : root(std::make_unique<OctreeNode<T>>(BoundingBox({-100000.0f, -100000.0f, -100000.0f}, {100000.0f, 100000.0f, 100000.0f}))),
          maxItems(8),
          maxDepth(8),
          itemCount(0) {}

    explicit Octree(const BoundingBox& bounds, int maxItems = 8, int maxDepth = 8)
        : root(std::make_unique<OctreeNode<T>>(bounds)),
          maxItems(maxItems),
          maxDepth(maxDepth),
          itemCount(0) {}

    // Insert item
    bool insert(const Vector3& position, const T& data) {
        OctreeData<T> item{position, data};
        bool success = insertIntoNode(root.get(), item);
        if (success) itemCount++;
        return success;
    }

    // Query by bounding box
    std::vector<OctreeData<T>> query(const BoundingBox& bounds) const {
        std::vector<OctreeData<T>> results;
        queryNode(root.get(), bounds, results);
        return results;
    }

    // Query by radius
    std::vector<OctreeData<T>> queryRadius(const Vector3& center, float radius) const {
        BoundingBox bounds{
            {center.x - radius, center.y - radius, center.z - radius},
            {center.x + radius, center.y + radius, center.z + radius}
        };

        auto candidates = query(bounds);
        float radiusSq = radius * radius;

        std::vector<OctreeData<T>> filtered;
        for (const auto& item : candidates) {
            if (distanceSquared(center, item.position) <= radiusSq)
                filtered.push_back(item);
        }
        return filtered;
    }

    // Remove by position + predicate
    bool remove(const Vector3& position, const std::function<bool(const T&)>& predicate) {
        bool result = removeFromNode(root.get(), position, predicate);
        if (result) itemCount--;
        return result;
    }

    // Clear
    void clear() {
        root = std::make_unique<OctreeNode<T>>(root->bounds);
        itemCount = 0;
    }

    // Get statistics
    OctreeStats getStats() const {
        OctreeStats stats;
        stats.totalItems = itemCount;
        collectStats(root.get(), stats);
        stats.averageItemsPerLeaf = stats.leafNodes > 0
            ? static_cast<float>(itemCount) / static_cast<float>(stats.leafNodes)
            : 0.f;
        return stats;
    }

private:
    std::unique_ptr<OctreeNode<T>> root;
    int maxItems;
    int maxDepth;
    size_t itemCount;

    // -------------------- Internal helpers --------------------

    bool insertIntoNode(OctreeNode<T>* node, const OctreeData<T>& item) {
        if (!containsPoint(node->bounds, item.position))
            return false;

        // Leaf with space
        if (node->isLeaf() && node->items.size() < static_cast<size_t>(maxItems)) {
            node->items.push_back(item);
            return true;
        }

        // Leaf at max depth
        if (node->isLeaf() && node->depth >= maxDepth) {
            node->items.push_back(item);
            return true;
        }

        // Subdivide if leaf and over capacity
        if (node->isLeaf()) {
            subdivide(node);
        }

        // Insert into children
        for (auto& child : node->children) {
            if (containsPoint(child->bounds, item.position))
                return insertIntoNode(child.get(), item);
        }

        return false;
    }

    void queryNode(const OctreeNode<T>* node, const BoundingBox& bounds,
                   std::vector<OctreeData<T>>& results) const {
        if (!intersects(node->bounds, bounds))
            return;

        for (const auto& item : node->items) {
            if (containsPoint(bounds, item.position))
                results.push_back(item);
        }

        if (!node->isLeaf()) {
            for (const auto& child : node->children)
                queryNode(child.get(), bounds, results);
        }
    }

    bool removeFromNode(OctreeNode<T>* node, const Vector3& position,
                        const std::function<bool(const T&)>& predicate) {
        if (!containsPoint(node->bounds, position))
            return false;

        auto it = std::find_if(node->items.begin(), node->items.end(),
            [&](const OctreeData<T>& item) {
                return item.position.x == position.x &&
                       item.position.y == position.y &&
                       item.position.z == position.z &&
                       predicate(item.data);
            });

        if (it != node->items.end()) {
            node->items.erase(it);
            return true;
        }

        if (!node->isLeaf()) {
            for (auto& child : node->children)
                if (removeFromNode(child.get(), position, predicate))
                    return true;
        }

        return false;
    }

    void subdivide(OctreeNode<T>* node) {
        const auto& min = node->bounds.min;
        const auto& max = node->bounds.max;
        Vector3 mid{ (min.x + max.x) / 2.f, (min.y + max.y) / 2.f, (min.z + max.z) / 2.f };

        node->children.reserve(8);
        node->children.push_back(std::make_unique<OctreeNode<T>>(BoundingBox{{min.x, min.y, min.z}, {mid.x, mid.y, mid.z}}, node->depth + 1));
        node->children.push_back(std::make_unique<OctreeNode<T>>(BoundingBox{{mid.x, min.y, min.z}, {max.x, mid.y, mid.z}}, node->depth + 1));
        node->children.push_back(std::make_unique<OctreeNode<T>>(BoundingBox{{min.x, min.y, mid.z}, {mid.x, mid.y, max.z}}, node->depth + 1));
        node->children.push_back(std::make_unique<OctreeNode<T>>(BoundingBox{{mid.x, min.y, mid.z}, {max.x, mid.y, max.z}}, node->depth + 1));
        node->children.push_back(std::make_unique<OctreeNode<T>>(BoundingBox{{min.x, mid.y, min.z}, {mid.x, max.y, mid.z}}, node->depth + 1));
        node->children.push_back(std::make_unique<OctreeNode<T>>(BoundingBox{{mid.x, mid.y, min.z}, {max.x, max.y, mid.z}}, node->depth + 1));
        node->children.push_back(std::make_unique<OctreeNode<T>>(BoundingBox{{min.x, mid.y, mid.z}, {mid.x, max.y, max.z}}, node->depth + 1));
        node->children.push_back(std::make_unique<OctreeNode<T>>(BoundingBox{{mid.x, mid.y, mid.z}, {max.x, max.y, max.z}}, node->depth + 1));

        // Redistribute existing items
        auto itemsToRedistribute = std::move(node->items);
        node->items.clear();

        for (const auto& item : itemsToRedistribute) {
            for (auto& child : node->children) {
                if (containsPoint(child->bounds, item.position)) {
                    insertIntoNode(child.get(), item);
                    break;
                }
            }
        }
    }

    // -------------------- Geometry helpers --------------------

    static bool containsPoint(const BoundingBox& b, const Vector3& p) {
        return (p.x >= b.min.x && p.x <= b.max.x &&
                p.y >= b.min.y && p.y <= b.max.y &&
                p.z >= b.min.z && p.z <= b.max.z);
    }

    static bool intersects(const BoundingBox& a, const BoundingBox& b) {
        return (a.min.x <= b.max.x && a.max.x >= b.min.x &&
                a.min.y <= b.max.y && a.max.y >= b.min.y &&
                a.min.z <= b.max.z && a.max.z >= b.min.z);
    }

    static float distanceSquared(const Vector3& a, const Vector3& b) {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        float dz = a.z - b.z;
        return dx * dx + dy * dy + dz * dz;
    }

    void collectStats(const OctreeNode<T>* node, OctreeStats& stats) const {
        stats.totalNodes++;
        stats.maxDepth = std::max(stats.maxDepth, node->depth);

        if (node->isLeaf()) {
            stats.leafNodes++;
        } else {
            for (const auto& child : node->children)
                collectStats(child.get(), stats);
        }
    }
};

