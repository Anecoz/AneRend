#include "PhysicsJoltImpl.h"

#include "../util/TransformHelpers.h"

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <thread>

namespace physics {

namespace {

glm::mat4 jphMatToGlm(JPH::RMat44Arg jph)
{
	glm::mat4 glmMat(1.0f);
	glmMat[0] = glm::vec4(jph.GetColumn4(0).GetX(), jph.GetColumn4(0).GetY(), jph.GetColumn4(0).GetZ(), jph.GetColumn4(0).GetW());
	glmMat[1] = glm::vec4(jph.GetColumn4(1).GetX(), jph.GetColumn4(1).GetY(), jph.GetColumn4(1).GetZ(), jph.GetColumn4(1).GetW());
	glmMat[2] = glm::vec4(jph.GetColumn4(2).GetX(), jph.GetColumn4(2).GetY(), jph.GetColumn4(2).GetZ(), jph.GetColumn4(2).GetW());
	glmMat[3] = glm::vec4(jph.GetColumn4(3).GetX(), jph.GetColumn4(3).GetY(), jph.GetColumn4(3).GetZ(), jph.GetColumn4(3).GetW());

	return glmMat;
}

// Return [quat, trans]
std::tuple<JPH::Quat, JPH::RVec3> glmToJPHRotTrans(const glm::mat4& transform)
{
	auto [quat, trans] = util::getRotAndTrans(transform);

	return { JPH::Quat(quat.x, quat.y, quat.z, quat.w) , JPH::RVec3(trans.x, trans.y, trans.z) };
}

void fillInBodySettings(
	JPH::BodyCreationSettings& bodySettings, 
	const glm::mat4& transform, 
	const JPH::Shape* shape, 
	const component::RigidBody& rigidComp)
{
	auto [quat, trans] = util::getRotAndTrans(transform);

	auto layer = rigidComp._motionType == rigidComp.Dynamic || rigidComp._motionType == rigidComp.Kinematic ? Layers::MOVING : Layers::NON_MOVING;
	auto motionType = rigidComp._motionType == rigidComp.Dynamic ? JPH::EMotionType::Dynamic : (rigidComp._motionType == rigidComp.Static ? JPH::EMotionType::Static : JPH::EMotionType::Kinematic);

	if (shape) bodySettings.SetShape(shape);
	bodySettings.mMotionType = motionType;
	bodySettings.mObjectLayer = layer;
	bodySettings.mPosition = JPH::RVec3(trans.x, trans.y, trans.z);
	bodySettings.mRotation = JPH::Quat(quat.x, quat.y, quat.z, quat.w);
	bodySettings.mFriction = rigidComp._friction;
	bodySettings.mRestitution = rigidComp._restitution;
	bodySettings.mLinearDamping = rigidComp._linearDamping;
	bodySettings.mAngularDamping = rigidComp._angularDamping;
	bodySettings.mGravityFactor = rigidComp._gravityFactor;

	if (rigidComp._motionType == rigidComp.Dynamic) {
		// Use supplied mass from the rigid body
		bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		bodySettings.mMassPropertiesOverride.mMass = rigidComp._mass;
	}
}

}

PhysicsJoltImpl::PhysicsJoltImpl()
	: _tempAllocator(nullptr)
	, _jobSystem(nullptr)
{
}

PhysicsJoltImpl::~PhysicsJoltImpl()
{
	for (auto& [_, character] : _charMap) {
		character->RemoveFromPhysicsSystem();
	}

	_charMap.clear();

	delete _tempAllocator;
	delete _jobSystem;
	JPH::UnregisterTypes();
	delete JPH::Factory::sInstance;
	JPH::Factory::sInstance = nullptr;
}

void PhysicsJoltImpl::staticInit()
{
	JPH::RegisterDefaultAllocator();
	JPH::Factory::sInstance = new JPH::Factory();
	JPH::RegisterTypes();
}

void physics::PhysicsJoltImpl::init()
{
	// Do the initial setup here
	_tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
	_jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

	_physicsSystem.Init(_maxBodies, _numBodyMutexes, _maxBodyPairs, _maxContactConstraints, _broadPhaseLayerIF, _objectVsBroadPhaseLayerFilter, _objectVsObjectLayerFilter);

	_physicsSystem.SetBodyActivationListener(&_activationListener);
	_physicsSystem.SetContactListener(&_contactListener);
}

void PhysicsJoltImpl::preUpdate(double delta)
{
	
}

void PhysicsJoltImpl::update(double delta)
{
	_physicsSystem.Update((float)delta, 1, _tempAllocator, _jobSystem);
}

void PhysicsJoltImpl::postUpdate(double delta)
{
	// TODO: Make cache efficient
	for (auto& [_, character] : _charMap) {
		character->PostSimulation(0.05f);
	}
}

std::vector<TransformSyncInfo> PhysicsJoltImpl::retrieveCurrentTransforms()
{
	std::vector<TransformSyncInfo> out;

	// For now do a simple loop over all current bodies and retrieve their transforms.
	// TODO: 
	// - Cache-friendly structure, like a vector
	// - parallell for loop
	// - take a lambda that is executed for each transform instead of returning the sync struct
	auto& bi = _physicsSystem.GetBodyInterface(); // locks
	for (auto& [node, bodyId] : _bodyMap) {

		if (!bi.IsActive(bodyId)) continue;

		TransformSyncInfo syncInfo{};
		syncInfo._node = node;

		auto transform = bi.GetCenterOfMassTransform(bodyId);
		glm::mat4 glmMat = jphMatToGlm(transform);

		syncInfo._global = std::move(glmMat);
		out.emplace_back(std::move(syncInfo));
	}

	// Characters
	for (auto& [node, character] : _charMap) {
		TransformSyncInfo syncInfo{};
		syncInfo._node = node;

		auto transform = character->GetWorldTransform();
		glm::mat4 glmMat = jphMatToGlm(transform);

		syncInfo._global = std::move(glmMat);
		out.emplace_back(std::move(syncInfo));
	}

	return out;
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

bool PhysicsJoltImpl::isBodyKnown(util::Uuid node) const
{
	return _bodyMap.find(node) != _bodyMap.end();
}

bool PhysicsJoltImpl::isShapeKnown(util::Uuid node) const
{
	return _shapeMap.find(node) != _shapeMap.end();
}

bool PhysicsJoltImpl::isCharKnown(util::Uuid node) const
{
	return _charMap.find(node) != _charMap.end();
}

void PhysicsJoltImpl::remove(util::Uuid node)
{
	bool isBody = _bodyMap.find(node) != _bodyMap.end();
	bool isChar = _charMap.find(node) != _charMap.end();
	auto& bi = _physicsSystem.GetBodyInterface();

	if (!isBody && !isChar) {
		printf("Physics cannot remove node %s, doesn't exist!\n", node.str().c_str());
		return;
	}

	if (isBody) {
		auto& bodyId = _bodyMap[node];
		bi.RemoveBody(bodyId);
		_bodyMap.erase(node);
	}
	else if (isChar) {
		removeChar(node);
	}
	_shapeMap.erase(node);
}

void PhysicsJoltImpl::removeChar(util::Uuid node)
{
	if (_charMap.find(node) == _charMap.end()) {
		printf("Physics cannot remove char %s, doesn't exist\n", node.str().c_str());
		return;
	}

	_charMap[node]->RemoveFromPhysicsSystem();
	_charMap.erase(node);
}

void PhysicsJoltImpl::addSphere(float radius, util::Uuid node)
{
	_shapeMap[node] = new JPH::SphereShape(radius);

	setShapeBodyOrChar(node);
}

void PhysicsJoltImpl::addBox(const glm::vec3& halfExtent, util::Uuid node)
{
	_shapeMap[node] = new JPH::BoxShape(JPH::Vec3(halfExtent.x, halfExtent.y, halfExtent.z));

	setShapeBodyOrChar(node);
}

void PhysicsJoltImpl::addCapsule(float halfHeight, float radius, util::Uuid node)
{
	_shapeMap[node] = new JPH::CapsuleShape(halfHeight, radius);

	setShapeBodyOrChar(node);
}

void PhysicsJoltImpl::addMesh(const render::asset::Mesh& mesh, util::Uuid node)
{
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
		printf("Error creating Jolt mesh: %s\n", res.GetError().c_str());
		return;
	}

	_shapeMap[node] = res.Get();

	setShapeBodyOrChar(node);
}

void PhysicsJoltImpl::addRigidBody(const component::RigidBody& rigidComp, const glm::mat4& transform, util::Uuid node)
{
	if (!isShapeKnown(node)) {
		return;
	}

	JPH::BodyCreationSettings settings;
	fillInBodySettings(settings, transform, _shapeMap[node], rigidComp);

	auto& bi = _physicsSystem.GetBodyInterface();
	_bodyMap[node] = bi.CreateAndAddBody(settings, JPH::EActivation::Activate);
}

void PhysicsJoltImpl::addCharacterController(const component::CharacterController& comp, const glm::mat4& transform, util::Uuid node)
{
	if (!isShapeKnown(node)) {
		printf("Physics cannot add a character without a shape!\n");
		return;
	}

	auto [quat, trans] = glmToJPHRotTrans(transform);

	JPH::Ref<JPH::CharacterSettings> settings = new JPH::CharacterSettings();
	settings->mMaxSlopeAngle = glm::radians(45.0f);
	settings->mLayer = Layers::MOVING;
	settings->mShape = _shapeMap[node];
	settings->mFriction = 0.5f;
	settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -0.3f);
	settings->mMass = comp._mass;
	auto character = new JPH::Character(settings, trans, quat, 0, &_physicsSystem);
	character->AddToPhysicsSystem();

	_charMap[node] = character;
}

void PhysicsJoltImpl::updateSphere(float radius, util::Uuid node)
{
	if (!isShapeKnown(node)) return;

	_shapeMap[node] = new JPH::SphereShape(radius);
	setShapeBodyOrChar(node);
}

void PhysicsJoltImpl::updateBox(const glm::vec3& halfExtent, util::Uuid node)
{
	if (!isShapeKnown(node)) return;

	_shapeMap[node] = new JPH::BoxShape(JPH::Vec3(halfExtent.x, halfExtent.y, halfExtent.z));
	setShapeBodyOrChar(node);
}

void PhysicsJoltImpl::updateCapsule(float halfHeight, float radius, util::Uuid node)
{
	if (!isShapeKnown(node)) return;

	_shapeMap[node] = new JPH::CapsuleShape(halfHeight, radius);
	setShapeBodyOrChar(node);
}

void PhysicsJoltImpl::updateRigidBody(const component::RigidBody& rigidComp, util::Uuid node)
{
	if (!isBodyKnown(node)) return;

	// The simplest thing to do here seems to be to recreate the entire body
	auto& bodyId = _bodyMap[node];
	auto& bi = _physicsSystem.GetBodyInterface();

	auto trans = bi.GetWorldTransform(bodyId);
	glm::mat4 glmTrans = jphMatToGlm(trans);

	// Remove old body
	bi.RemoveBody(bodyId);

	addRigidBody(rigidComp, glmTrans, node);
}

void PhysicsJoltImpl::updateCharacterController(const component::CharacterController& charComp, util::Uuid node)
{
	if (!isCharKnown(node)) return;

	// Have to recreate it
	auto trans = _charMap[node]->GetWorldTransform();
	glm::mat4 glmTrans = jphMatToGlm(trans);

	_charMap[node]->RemoveFromPhysicsSystem();

	addCharacterController(charComp, glmTrans, node);
}

void PhysicsJoltImpl::setPositionAndOrientation(const util::Uuid& node, glm::quat rot, glm::vec3 trans)
{
	if (!isShapeKnown(node)) return;

	if (isBodyKnown(node)) {
		auto& bodyId = _bodyMap[node];

		auto& bi = _physicsSystem.GetBodyInterface();
		bi.SetPositionAndRotationWhenChanged(bodyId, JPH::Vec3(trans.x, trans.y, trans.z), JPH::Quat(rot.x, rot.y, rot.z, rot.w), JPH::EActivation::Activate);
	}
	else if (isCharKnown(node)) {
		_charMap[node]->SetPositionAndRotation(JPH::Vec3(trans.x, trans.y, trans.z), JPH::Quat(rot.x, rot.y, rot.z, rot.w));
	}
}

void PhysicsJoltImpl::setDesiredVelocity(const glm::vec3& desiredVel, float jumpSpeed, float speed, const util::Uuid& node)
{
	if (!isCharKnown(node)) {
		printf("Physics cannot set desired velocity on %s, doesn't exist!\n", node.str().c_str());
		return;
	}

	auto character = _charMap[node];

	// Copied from Jolt example:

	// Cancel movement in opposite direction of normal when touching something we can't walk up
	JPH::Vec3 movement_direction(desiredVel.x, desiredVel.y, desiredVel.z);
	JPH::Character::EGroundState ground_state = character->GetGroundState();
	if (ground_state == JPH::Character::EGroundState::OnSteepGround
		|| ground_state == JPH::Character::EGroundState::NotSupported) {
		JPH::Vec3 normal = character->GetGroundNormal();
		normal.SetY(0.0f);
		float dot = normal.Dot(movement_direction);
		if (dot < 0.0f)
			movement_direction -= (dot * normal) / normal.LengthSq();
	}

	if (character->IsSupported())	{
		// Update velocity
		JPH::Vec3 current_velocity = character->GetLinearVelocity();
		JPH::Vec3 desired_velocity = speed * movement_direction;
		desired_velocity.SetY(current_velocity.GetY());
		JPH::Vec3 new_velocity = 0.95f * current_velocity + 0.05f * desired_velocity;

		// Jump
		if (false && ground_state == JPH::Character::EGroundState::OnGround)
			new_velocity += JPH::Vec3(0, jumpSpeed, 0);

		// Update the velocity
		character->SetLinearVelocity(new_velocity);
	}
}

void PhysicsJoltImpl::debugSphere()
{
	auto& bi = _physicsSystem.GetBodyInterface();

	JPH::BodyCreationSettings sphereSettings(new JPH::SphereShape(0.5f), JPH::RVec3(16.0f, 15.0f, 16.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
	JPH::BodyID sphereId = bi.CreateAndAddBody(sphereSettings, JPH::EActivation::Activate);
}

void PhysicsJoltImpl::setShapeBodyOrChar(const util::Uuid& node)
{
	if (isBodyKnown(node)) {
		auto& bi = _physicsSystem.GetBodyInterface();
		bi.SetShape(_bodyMap[node], _shapeMap[node], false, JPH::EActivation::Activate);

		if (!bi.IsAdded(_bodyMap[node])) {
			bi.AddBody(_bodyMap[node], JPH::EActivation::Activate);
		}
	}
	else if (isCharKnown(node)) {
		_charMap[node]->SetShape(_shapeMap[node], FLT_MAX);
	}
}

}


