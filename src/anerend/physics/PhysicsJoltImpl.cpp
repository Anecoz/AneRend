#include "PhysicsJoltImpl.h"

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include <thread>

namespace physics {

PhysicsJoltImpl::PhysicsJoltImpl()
	: _tempAllocator(nullptr)
	, _jobSystem(nullptr)
{
}

PhysicsJoltImpl::~PhysicsJoltImpl()
{
	delete _tempAllocator;
	delete _jobSystem;
	JPH::UnregisterTypes();
	delete JPH::Factory::sInstance;
	JPH::Factory::sInstance = nullptr;
}

void physics::PhysicsJoltImpl::init()
{
	// Do the initial setup here
	JPH::RegisterDefaultAllocator();
	JPH::Factory::sInstance = new JPH::Factory();
	JPH::RegisterTypes();

	_tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
	_jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

	_physicsSystem.Init(_maxBodies, _numBodyMutexes, _maxBodyPairs, _maxContactConstraints, _broadPhaseLayerIF, _objectVsBroadPhaseLayerFilter, _objectVsObjectLayerFilter);

	_physicsSystem.SetBodyActivationListener(&_activationListener);
	_physicsSystem.SetContactListener(&_contactListener);
}

void PhysicsJoltImpl::update(double delta)
{
	_physicsSystem.Update((float)delta, 1, _tempAllocator, _jobSystem);
}

void PhysicsJoltImpl::debugDraw(JPH::DebugRenderer* renderer)
{
	JPH::BodyManager::DrawSettings drawSettings{};
	drawSettings.mDrawBoundingBox = true;
	drawSettings.mDrawShapeWireframe = true;
	_physicsSystem.DrawBodies(drawSettings, renderer);

	_physicsSystem.DrawConstraints(renderer);
	_physicsSystem.DrawConstraintLimits(renderer);
	_physicsSystem.DrawConstraintReferenceFrame(renderer);
}

void PhysicsJoltImpl::remove(util::Uuid node)
{
}

namespace {

inline float convertUint16ToFloat(std::uint16_t v, float scale = 5.0f)
{
	float zeroToOne = (float)v / (float)65535;

	return zeroToOne * scale;
}

}

void PhysicsJoltImpl::addHeightfield(const render::asset::Mesh& mesh, util::Uuid node)
{
	auto& bi = _physicsSystem.GetBodyInterface();

	// Prepare data so that JPH can eat it
	JPH::IndexedTriangleList indices;
	indices.resize(mesh._indices.size() / 3);
	for (std::size_t i = 0; i < mesh._indices.size() / 3; ++i) {
		JPH::IndexedTriangle tri(mesh._indices[i * 3 + 0], mesh._indices[i * 3 + 1], mesh._indices[i * 3 + 2]);
		indices[i] = std::move(tri);
	}

	JPH::VertexList verts;
	verts.reserve(mesh._vertices.size());
	for (std::size_t i = 0; i < mesh._vertices.size(); ++i) {
		JPH::Float3 v(mesh._vertices[i].pos.x, mesh._vertices[i].pos.y, mesh._vertices[i].pos.z);
		verts.emplace_back(std::move(v));
	}

	JPH::MeshShapeSettings settings(std::move(verts), std::move(indices));
	auto res = settings.Create();

	if (res.HasError()) {
		printf("Error creating Jolt heightfield: %s\n", res.GetError().c_str());
		return;
	}

	auto shape = res.Get();

	// TODO: Transform
	JPH::BodyCreationSettings bodySettings(shape, JPH::Vec3(0.0f, 0.0f, 0.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);
	JPH::Body* body = bi.CreateBody(bodySettings);

	bi.AddBody(body->GetID(), JPH::EActivation::Activate);
}

void PhysicsJoltImpl::debugSphere()
{
	auto& bi = _physicsSystem.GetBodyInterface();

	JPH::BodyCreationSettings sphereSettings(new JPH::SphereShape(0.5f), JPH::RVec3(16.0f, 15.0f, 16.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
	JPH::BodyID sphereId = bi.CreateAndAddBody(sphereSettings, JPH::EActivation::Activate);
}

}


