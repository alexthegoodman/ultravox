#include "PhysicsSystem.h"
#include "Logger.h"

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

    LOG("Initializing Jolt Physics... 2a");

    // Create a factory
    JPH::Factory::sInstance = new JPH::Factory();

    LOG("Initializing Jolt Physics... a");

    // Register all Jolt physics types
    JPH::RegisterTypes();

    LOG("Initializing Jolt Physics... b");

    // We need a temp allocator for temporary allocations during the physics update. We're using a fixed size allocator.
    tempAllocator = new JPH::TempAllocatorMalloc();

    LOG("Initializing Jolt Physics... c");

    // We need a job system that will execute physics jobs on multiple threads.
    // We use the default implementation for the job system.
    jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

    LOG("Initializing Jolt Physics... d");

    // Create the physics system
    physicsSystem = new JPH::PhysicsSystem();
    physicsSystem->Init(1024, 0, 1024, 1024, broadPhaseLayerInterface, objectVsBroadphaseLayerFilter, objectVsObjectLayerFilter);

    LOG("Initializing Jolt Physics... 3");

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
