#pragma once

#include "../component/Components.h"
#include "../render/asset/Texture.h" // For heightfields
#include "../render/asset/Mesh.h"

#include <Jolt/Jolt.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Renderer/DebugRenderer.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Character/Character.h>

#include <glm/glm.hpp>

#include <unordered_map>
#include <iostream>

namespace physics {

struct TransformSyncInfo
{
	util::Uuid _node;
	glm::mat4 _global;
};

namespace Layers
{
	static constexpr JPH::ObjectLayer NON_MOVING = 0;
	static constexpr JPH::ObjectLayer MOVING = 1;
	static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
	{
		switch (inObject1)
		{
		case Layers::NON_MOVING:
			return inObject2 == Layers::MOVING; // Non moving only collides with moving
		case Layers::MOVING:
			return true; // Moving collides with everything
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace BroadPhaseLayers
{
	static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
	static constexpr JPH::BroadPhaseLayer MOVING(1);
	static constexpr JPH::uint NUM_LAYERS(2);
};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
	BPLayerInterfaceImpl()
	{
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	virtual JPH::uint GetNumBroadPhaseLayers() const override
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
	{
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}

#if defined(_DEBUG)
	virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
	{
		switch ((JPH::BroadPhaseLayer::Type)inLayer)
		{
		case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
		case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
		default:													JPH_ASSERT(false); return "INVALID";
		}
	}
#endif

private:
	JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
	{
		switch (inLayer1)
		{
		case Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::MOVING:
			return true;
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

// An example contact listener
class MyContactListener : public JPH::ContactListener
{
public:
	// See: ContactListener
	virtual JPH::ValidateResult	OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override
	{
		//std::cout << "Contact validate callback" << std::endl;

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
	{
		//std::cout << "A contact was added" << std::endl;
	}

	virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
	{
		//std::cout << "A contact was persisted" << std::endl;
	}

	virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
	{
		//std::cout << "A contact was removed" << std::endl;
	}
};

// An example activation listener
class MyBodyActivationListener : public JPH::BodyActivationListener
{
public:
	virtual void OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override
	{
		//std::cout << "A body got activated" << std::endl;
	}

	virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override
	{
		//std::cout << "A body went to sleep" << std::endl;
	}
};

class PhysicsJoltImpl
{
public:
  PhysicsJoltImpl();
  ~PhysicsJoltImpl();

  PhysicsJoltImpl(const PhysicsJoltImpl&) = delete;
  PhysicsJoltImpl& operator=(const PhysicsJoltImpl&) = delete;
  PhysicsJoltImpl(PhysicsJoltImpl&&) = delete;
  PhysicsJoltImpl& operator=(PhysicsJoltImpl&&) = delete;

	static void staticInit();
  void init();

	void preUpdate(double delta);

	// Steps the simulation
	void update(double delta);

	void postUpdate(double delta);

	// Set velocities of all known bodies (and character controllers) to 0.
	void resetVelocities();

	// Upstream: Syncs the physics world for all known and active bodies
	std::vector<TransformSyncInfo> retrieveCurrentTransforms();

	// Draws as much debug information as possible about the current state (immediate mode)
	void debugDraw(JPH::DebugRenderer* renderer);

	// Returns if we have a body the given node
	bool isBodyKnown(util::Uuid node) const;

	// Returns if we have a shape for the given node
	bool isShapeKnown(util::Uuid node) const;

	// Do we have a character controller for this node
	bool isCharKnown(util::Uuid node) const;

	// Will remove shape and body/char for the given node
	void remove(util::Uuid node);

	// Will remove the character completely
	void removeChar(util::Uuid node);

	// Add colliders
	void addSphere(float radius, util::Uuid node);
	void addBox(const glm::vec3& halfExtent, util::Uuid node);
	void addCapsule(float halfHeight, float radius, util::Uuid node);
	void addMesh(const render::asset::Mesh& mesh, util::Uuid node);

	// Add body
	void addRigidBody(const component::RigidBody& rigidComp, const glm::mat4& transform, util::Uuid node);

	// Add character controllers.
	void addCharacterController(const component::CharacterController& comp, const glm::mat4& transform, util::Uuid node);

	// Update colliders
	void updateSphere(float radius, util::Uuid node);
	void updateBox(const glm::vec3& halfExtent, util::Uuid node);
	void updateCapsule(float halfHeight, float radius, util::Uuid node);

	// Update bodies/chars
	void updateRigidBody(const component::RigidBody& rigidComp, util::Uuid node);
	void updateCharacterController(const component::CharacterController& charComp, util::Uuid node);

	// Downstream: Syncs a given body with a new orientation and translation.
	void setPositionAndOrientation(const util::Uuid& node, glm::quat rot, glm::vec3 trans);

	// Sets desired velocity of a char. Needs to be called before stepping the simulation!
	void setDesiredVelocity(const glm::vec3& desiredVel, float jumpSpeed, float speed, const util::Uuid& node);

	void debugSphere();

private:
	void setShapeBodyOrChar(const util::Uuid& node);

	std::unordered_map<util::Uuid, JPH::BodyID> _bodyMap;
	std::unordered_map<util::Uuid, JPH::Ref<JPH::Shape>> _shapeMap;
	std::unordered_map<util::Uuid, JPH::Ref<JPH::Character>> _charMap;

	JPH::TempAllocatorImpl* _tempAllocator = nullptr;
	JPH::JobSystemThreadPool* _jobSystem = nullptr;

	// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const unsigned _maxBodies = 1024;

	// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
	const unsigned _numBodyMutexes = 0;

	// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
	// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
	// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const unsigned _maxBodyPairs = 1024;

	// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
	// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
	const unsigned _maxContactConstraints = 1024;

	// Create mapping table from object layer to broadphase layer
	physics::BPLayerInterfaceImpl _broadPhaseLayerIF;

	// Create class that filters object vs broadphase layers
	ObjectVsBroadPhaseLayerFilterImpl _objectVsBroadPhaseLayerFilter;

	// Create class that filters object vs object layers
	ObjectLayerPairFilterImpl _objectVsObjectLayerFilter;

	MyBodyActivationListener _activationListener;
	MyContactListener _contactListener;

	JPH::PhysicsSystem _physicsSystem;
};

}