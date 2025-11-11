#include "PhysicsSystem.h"
#include "Logger.h"

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
// #include <Jolt/Physics/Collision/CastRay.h>
#include <Jolt/Physics/Collision/CollisionCollector.h> // Added for ClosestHitCollisionCollector
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Core/NonCopyable.h>
#include <Jolt/Physics/Collision/TransformedShape.h>
#include <Jolt/Physics/Collision/Shape/SubShapeID.h>

// Callback for asserts, where we can choose to break or not
// NOTE: I DO NOT want logs for physics bodies!
// static void TraceImpl(const char *inFMT, ...) {
//     // Format the message
//     va_list list;
//     va_start(list, inFMT);
//     char buffer[1024];
//     vsnprintf(buffer, sizeof(buffer), inFMT, list);
//     va_end(list);

//     // Output to the VS output pane
//     LOG(std::string("JoltPhysics: ") + buffer);
// }

#ifdef JPH_ENABLE_ASSERTS
// Callback for asserts, where we can choose to break or not
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, JPH::uint inLine) {
    LOG(std::string("JoltPhysics Assert Failed: ") + (inMessage != nullptr ? inMessage : "") + " in " + inFile + ":" + std::to_string(inLine));
    // Break in the debugger
    return true;
}
#endif // JPH_ENABLE_ASSERTS

PhysicsSystem::PhysicsSystem()
    : tempAllocator(nullptr),
      jobSystem(nullptr),
      physicsSystem(nullptr) {
}

PhysicsSystem::~PhysicsSystem() {
    shutdown();
}

void PhysicsSystem::init() {
    LOG("Initializing Jolt Physics... 1");

    // Register allocation hook
    // NOTE: are these necessary? cause JPH::DefaultAllocate does not exist, neither do the others
    // JPH::Allocate = JPH::DefaultAllocate;
    // JPH::Free = JPH::DefaultFree;
    // JPH::AlignedAllocate = JPH::DefaultAlignedAllocate;
    // JPH::AlignedFree = JPH::DefaultAlignedFree;

    // Install callbacks
    // JPH::Trace = TraceImpl;
#ifdef JPH_ENABLE_ASSERTS
    JPH::AssertFailed = AssertFailedImpl;
#endif // JPH_ENABLE_ASSERTS

    LOG("Initializing Jolt Physics... 2");

    JPH::RegisterDefaultAllocator();

    // Create a factory
    JPH::Factory::sInstance = new JPH::Factory();

    // Register all Jolt physics types
    JPH::RegisterTypes();

    // We need a temp allocator for temporary allocations during the physics update. We're using a fixed size allocator.
    tempAllocator = new JPH::TempAllocatorMalloc();

    // We need a job system that will execute physics jobs on multiple threads.
    // We use the default implementation for the job system.
    jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

    // Create the physics system
    physicsSystem = new JPH::PhysicsSystem();

    int maxBodies = 2048;

    physicsSystem->Init(maxBodies, 0, maxBodies, maxBodies, broadPhaseLayerInterface, objectVsBroadphaseLayerFilter, objectVsObjectLayerFilter);

    // A body activation listener gets notified when bodies activate and deactivate
    physicsSystem->SetBodyActivationListener(&bodyActivationListener);

    // A contact listener gets notified when bodies collide and starts or stops touching
    physicsSystem->SetContactListener(&contactListener);

    // Optimize broad phase
    physicsSystem->OptimizeBroadPhase();

    LOG("Jolt Physics Initialized");
}

void PhysicsSystem::update(float deltaTime, int collisionSteps) {
    if (physicsSystem) {
        // Next step
        physicsSystem->Update(deltaTime, collisionSteps, tempAllocator, jobSystem);
    }
}

void PhysicsSystem::shutdown() {
    LOG("Shutting down Jolt Physics");

    if (physicsSystem) {
        delete physicsSystem;
        physicsSystem = nullptr;
    }

    if (jobSystem) {
        delete jobSystem;
        jobSystem = nullptr;
    }

    if (tempAllocator) {
        delete tempAllocator;
        tempAllocator = nullptr;
    }

    // Unregisters all types created by the factory and frees the factory
    JPH::UnregisterTypes();
    if (JPH::Factory::sInstance) {
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    LOG("Jolt Physics Shut down");
}

JPH::BodyID PhysicsSystem::createBoxBody(const glm::vec3& position, const glm::vec3& halfExtent, JPH::EMotionType motionType, JPH::ObjectLayer objectLayer) {
    if (!physicsSystem) {
        LOG("PhysicsSystem not initialized. Cannot create body.");
        return JPH::BodyID();
    }

        // Create a box shape

        JPH::BoxShapeSettings boxShapeSettings(toJPHVec3(halfExtent));

        JPH::ShapeSettings::ShapeResult shapeResult = boxShapeSettings.Create();

        if (shapeResult.HasError()) {

            LOG("Failed to create box shape: " + std::string(shapeResult.GetError().c_str()));

            return JPH::BodyID();

        }
    JPH::ShapeRefC boxShape = shapeResult.Get();

    // Create the body
    JPH::BodyCreationSettings bodySettings(boxShape, toJPHVec3(position), JPH::Quat::sIdentity(), motionType, objectLayer);
    JPH::Body* body = physicsSystem->GetBodyInterface().CreateBody(bodySettings);
    
    if (body) {
        physicsSystem->GetBodyInterface().AddBody(body->GetID(), JPH::EActivation::Activate);
        return body->GetID();
    }
    return JPH::BodyID();
}

void PhysicsSystem::destroyBody(JPH::BodyID bodyID) {
    if (physicsSystem && !bodyID.IsInvalid()) {
        physicsSystem->GetBodyInterface().RemoveBody(bodyID);
        physicsSystem->GetBodyInterface().DestroyBody(bodyID);
    }
}

JPH::Character* PhysicsSystem::createCharacter(const glm::vec3& position) {
    if (!physicsSystem) {
        LOG("PhysicsSystem not initialized. Cannot create character.");
        return nullptr;
    }

    // 1. Define shape settings (e.g., a capsule)
    float character_height = 0.5f;
    float character_radius = 0.5f;
    JPH::Ref<JPH::ShapeSettings> shape_settings = new JPH::CapsuleShapeSettings(character_height / 2.0f, character_radius);

    // 2. Create the shape (this "cooks" the shape into an optimized format)
    JPH::ShapeSettings::ShapeResult shape_result = shape_settings->Create();
    if (shape_result.HasError()) {
        // Handle error
        // ...
    }

    // 3. Get the resulting shape and assign it to character settings
    JPH::Ref<JPH::Shape> character_shape = shape_result.Get();

    JPH::Ref<JPH::CharacterSettings> settings = new JPH::CharacterSettings();
    settings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);
    settings->mLayer = ObjectLayer::MOVING;
    settings->mShape = character_shape;
    settings->mFriction = 0.2f;
    settings->mGravityFactor = 1.0f;

    JPH::Vec3 joltVec = toJPHVec3(position);

    LOG("Provided position immediately before creation: " + std::to_string(joltVec.GetX()) + " and " + std::to_string(joltVec.GetY()) + " and " + std::to_string(joltVec.GetZ()));

    JPH::Character* character = new JPH::Character(settings, joltVec, JPH::Quat::sIdentity(), 0, physicsSystem);
    character->AddToPhysicsSystem(JPH::EActivation::Activate);

    character->SetPosition(joltVec);

    LOG("Jolt position immediately after creation: " + std::to_string(character->GetPosition().GetX()) + " " + std::to_string(character->GetPosition().GetY()) + " " + std::to_string(character->GetPosition().GetZ()));
    
    return character;
}

void PhysicsSystem::destroyCharacter(JPH::Character* character) {
    if (character) {
        character->RemoveFromPhysicsSystem();
        delete character;
    }
}

// PhysicsSystem::RayCastResult PhysicsSystem::castRay(const glm::vec3& origin, const glm::vec3& direction) {
//     RayCastResult result;

//     // Convert glm vectors to Jolt vectors
//     JPH::RVec3 joltOrigin = toJPHVec3(origin);
//     JPH::Vec3 joltDirection = toJPHVec3(direction);

//     // Create Jolt ray
//     // JPH::RayCast ray(joltOrigin, joltDirection * 1000.0f); // Multiply by a large number for ray length

//     // // Create a collision collector
//     // JPH::ClosestHitCollisionCollector<JPH::RayCastBodyCollector> collector;
//     // // JPH::RayCastBodyCollector collector;

//     // // Cast the ray using the broad phase query
//     // // physicsSystem->GetBroadPhaseQuery().CastRay(ray, collector);

//     // // if (collector.HadHit()) {
//     // //     result.hasHit = true;
//     // //     const JPH::TransformedShape* joltResult = collector.GetContext();
//     // //     result.bodyID = joltResult->mBodyID;

//     // //     const JPH::Vec3 hitPosition = ray.GetPointOnRay(collector.GetEarlyOutFraction());
        
//     // //     // Calculate hit position
//     // //     result.hitPosition = toGLMVec3(hitPosition);

//     // //     // Get the normal at the hit point
//     // //     // physicsSystem->GetBodyInterface().GetWorldSpaceSurfaceNormal(joltResult->mBodyID, joltResult.mSubShapeID2, hitPosition, joltNormal);
//     // //     // const JPH::BodyLockInterfaceLocking bodyLockInterface = physicsSystem->GetBodyLockInterface();
//     // //     JPH::Vec3 joltNormal = joltResult->GetWorldSpaceSurfaceNormal(joltResult.sub, hitPosition);
//     // //     result.hitNormal = toGLMVec3(joltNormal);
//     // // }

//     // collector.Reset();
//     // // physicsSystem->GetBroadPhaseQuery().CastRay(ray, collector);
//     // physicsSystem->GetNarrowPhaseQuery().CastRay();

//     // if ( collector.HadHit() )
//     // {
//     //     // Access the result
//     //     const auto & hit = collector.mHit;
//     //     // hit is of type CollectorType::ResultType

//     //     // Compute hit position on the ray
//     //     float fraction = hit.mFraction;
//     //     JPH::Vec3 joltHitPos = ray.GetPointOnRay(fraction);

//     //     // Lock the body for read access
//     //     JPH::BodyLockRead lock(physicsSystem->GetBodyLockInterface(), hit.mBodyID);

//     //     if (lock.Succeeded())
//     //     {
//     //         const JPH::Body & body = lock.GetBody();

//     //         // Use SubShapeID2 from the hit result
//     //         JPH::Vec3 joltNormal = body.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, joltHitPos);
            
//     //         // convert to glm etc.
//     //         result.hasHit = true;
//     //         result.bodyID = hit.mBodyID;
//     //         result.hitPosition = toGLMVec3(joltHitPos);
//     //         result.hitNormal = toGLMVec3(joltNormal);
//     //     }
//     // }

//     JPH::RayCast ray(joltOrigin, joltDirection * 1000.0f);

//     // Default raycast settings
//     JPH::RayCastSettings rayCastSettings;
//     // rayCastSettings.mBackFaceMode = JPH::EBackFaceMode::IgnoreBackFaces; // typical choice

//     // Collector for the closest hit
//     JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> collector;

//     // Perform *narrow phase* raycast (not broad phase)
//     physicsSystem->GetNarrowPhaseQuery().CastRay(
//         ray,
//         rayCastSettings,
//         collector
//         // {}, // BroadPhaseLayerFilter
//         // {}, // ObjectLayerFilter
//         // {}, // BodyFilter
//         // {}  // ShapeFilter
//     );

//     if (collector.HadHit())
//     {
//         const JPH::RayCastResult &hit = collector.mHit;
//         JPH::RVec3 joltHitPos = ray.GetPointOnRay(hit.mFraction);

//         // Lock the body
//         JPH::BodyLockRead lock(physicsSystem->GetBodyLockInterface(), hit.mBodyID);

//         if (lock.Succeeded())
//         {
//             const JPH::Body &body = lock.GetBody();
//             JPH::Vec3 joltNormal = body.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, joltHitPos);

//             result.hasHit = true;
//             result.bodyID = hit.mBodyID;
//             result.hitPosition = toGLMVec3(joltHitPos);
//             result.hitNormal = toGLMVec3(joltNormal);
//         }
//     }


//     return result;
// }

PhysicsSystem::RayCastResult PhysicsSystem::castRay(const glm::vec3& origin, const glm::vec3& direction)
{
    RayCastResult result;

    if (!physicsSystem)
        return result;

    // Convert glm → Jolt
    JPH::RVec3 joltOrigin = toJPHVec3(origin);
    JPH::Vec3 joltDirection = toJPHVec3(direction);

    // Define the ray (RRayCast uses double precision)
    JPH::RRayCast ray(joltOrigin, joltDirection * 1000.0f);

    // Struct to receive the hit
    JPH::RayCastResult hit;

    // Narrow phase query – simple single-hit version
    bool foundHit = physicsSystem->GetNarrowPhaseQuery().CastRay(
        ray,
        hit,
        {}, // BroadPhaseLayerFilter
        {}, // ObjectLayerFilter
        {}  // BodyFilter
    );

    if (foundHit)
    {
        JPH::RVec3 joltHitPos = ray.GetPointOnRay(hit.mFraction);

        // Lock the body for reading
        JPH::BodyLockRead lock(physicsSystem->GetBodyLockInterface(), hit.mBodyID);
        if (lock.Succeeded())
        {
            const JPH::Body& body = lock.GetBody();
            JPH::Vec3 joltNormal = body.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, joltHitPos);

            result.hasHit      = true;
            result.bodyID      = hit.mBodyID;
            result.hitPosition = toGLMVec3(joltHitPos);
            result.hitNormal   = toGLMVec3(joltNormal);
        }
    }

    return result;
}
