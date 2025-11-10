#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include <glm/glm.hpp>

// All Jolt includes can be compiled only once (one .cpp file should include this).
// This is done in PhysicsSystem.cpp

// We will use a single temp allocator for the entire physics system.
// B.O.W. (Before Our World)
namespace JPH {
    class TempAllocator;
    class JobSystem;
}

// Layer that objects can be in / interact with (e.g. "non-moving", "moving")
namespace ObjectLayer {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};

// Each broadphase layer will only collide with certain other broadphase layers
namespace BroadPhaseLayer {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::BroadPhaseLayer NUM_LAYERS(2);
};

/// Class that helps determine if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
        switch (inObject1) {
            case ObjectLayer::NON_MOVING:
                return inObject2 == ObjectLayer::MOVING; // Non-moving only collides with moving
            case ObjectLayer::MOVING:
                return true; // Moving collides with everything
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

/// Class that helps determine if two broadphase layers can collide
class BPLayerOverrideImpl : public JPH::BroadPhaseLayerFilter {
public:
    virtual bool ShouldCollide(JPH::BroadPhaseLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const {
        if (inLayer1 == BroadPhaseLayer::NON_MOVING)
            return inLayer2 == BroadPhaseLayer::MOVING;
        return true;
    }
};

// An example contact listener
class MyContactListener : public JPH::ContactListener {
public:
    // See: ContactListener
    virtual JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override {
        // std::cout << "Contact validate callback" << std::endl;
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override {
        // std::cout << "A contact was added" << std::endl;
    }

    virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override {
        // std::cout << "A contact was persisted" << std::endl;
    }

    virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override {
        // std::cout << "A contact was removed" << std::endl;
    }
};

// An example body activation listener
class MyBodyActivationListener : public JPH::BodyActivationListener {
public:
    virtual void OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override {
        // std::cout << "A body got activated" << std::endl;
    }

    virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override {
        // std::cout << "A body got deactivated" << std::endl;
    }
};

class PhysicsSystem {
public:
    PhysicsSystem();
    ~PhysicsSystem();

    void init();
    void update(float deltaTime, int collisionSteps);
    void shutdown();

    JPH::BodyID createBoxBody(const glm::vec3& position, const glm::vec3& halfExtent, JPH::EMotionType motionType, JPH::ObjectLayer objectLayer);
    void destroyBody(JPH::BodyID bodyID);

    // Helper to convert glm::vec3 to JPH::Vec3
    static JPH::Vec3 toJPHVec3(const glm::vec3& v) {
        return JPH::Vec3(v.x, v.y, v.z);
    }

    // Helper to convert JPH::Vec3 to glm::vec3
    static glm::vec3 toGLMVec3(const JPH::Vec3& v) {
        return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
    }

    // Helper to convert JPH::Vec3 to glm::vec3
    static glm::vec3 toGLMVec3(const Vector3& v) {
        return glm::vec3(v.x, v.y, v.z);
    }

private:
    JPH::TempAllocator* tempAllocator;
    JPH::JobSystemThreadPool* jobSystem;
    JPH::PhysicsSystem* physicsSystem;

    ObjectLayerPairFilterImpl objectLayerPairFilter;
    BPLayerOverrideImpl bpLayerOverride;
    MyContactListener contactListener;
    MyBodyActivationListener bodyActivationListener;
};
