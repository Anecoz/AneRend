#include "PhysicsSystem.h"

#include "../render/scene/Scene.h"

#include "../util/TransformHelpers.h"

#include "../component/Components.h"
#include "../component/Registry.h"

#include "PhysicsJoltImpl.h"
#include "JoltDebugRenderer.h"

namespace physics {

PhysicsSystem::PhysicsSystem(component::Registry* registry, render::scene::Scene* scene, render::RenderContext* rc)
  : _debugRenderer(nullptr)
  , _registry(registry)
  , _scene(scene)
  , _rc(rc)
{
  connectObserver();
}

PhysicsSystem::~PhysicsSystem()
{
  delete _joltImpl;
  delete _debugRenderer;
}

void PhysicsSystem::setRegistry(component::Registry* registry)
{
  _registry = registry;
  connectObserver();
  _goThroughEverything = true;
}

void PhysicsSystem::setScene(render::scene::Scene* scene)
{
  _scene = scene;
  _goThroughEverything = true;
}

void PhysicsSystem::init()
{
  PhysicsJoltImpl::staticInit();
  _joltImpl = new PhysicsJoltImpl();
  _joltImpl->init();
  _debugRenderer = new JoltDebugRenderer(_rc);
  _debugRenderer->init();
}

void PhysicsSystem::downstreamTransformSync()
{
  // Update only static and kinematic objects, but allow dynamic aswell if simulation is paused
  for (auto ent : _transformObserver) {
    auto node = _registry->reverseLookup(ent);

    auto& transComp = _registry->getComponent<component::Transform>(node);
    auto [rot, trans] = util::getRotAndTrans(transComp._globalTransform);

    if (_registry->hasComponent<component::RigidBody>(node)) {
      auto& rigidComp = _registry->getComponent<component::RigidBody>(node);
      auto mt = rigidComp._motionType;

      if (mt == rigidComp.Kinematic || mt == rigidComp.Static || (!_simulationRunning && mt == rigidComp.Dynamic)) {
        _joltImpl->setPositionAndOrientation(node, std::move(rot), std::move(trans));
      }
    }
    else if (_registry->hasComponent<component::CharacterController>(node)) {
      _joltImpl->setPositionAndOrientation(node, std::move(rot), std::move(trans));
    }
  }

  _transformObserver.clear();
}

void PhysicsSystem::update(double delta, bool debugDraw)
{
  // In order to create a body (softbody or rigidbody), we also need a shape.
  // So we have to wait for a node that comes in here (via update, not a per-frame view)
  // that has both.

  auto rigidNodeLambda = [this](util::Uuid node) {
    checkIfCreate(node);

    // Check if we are known, and in that case update the rigid body
    if (_joltImpl->isBodyKnown(node)) {
      auto& rigidComp = _registry->getComponent<component::RigidBody>(node);
      _joltImpl->updateRigidBody(rigidComp, node);
    }
  };

  auto charNodeLambda = [this](util::Uuid node) {
    checkIfCreate(node);

    // Check if we are known, and in that case update controller
    // TODO: Currently nothing to update here
    //if (_joltImpl->isCharKnown(node)) {
      //auto& charComp = _registry->getComponent<component::CharacterController>(node);
    //}
  };

  // Check rigidbodies (softbodies TODO) and check if they have colliders
  // If they are known already, update parameters of the body and/or collider
  if (_goThroughEverything) {
    _goThroughEverything = false;

    auto rigidView = _registry->getEnttRegistry().view<component::RigidBody>();
    for (auto ent : rigidView) {
      auto node = _registry->reverseLookup(ent);
      rigidNodeLambda(node);
    }

    auto charView = _registry->getEnttRegistry().view<component::CharacterController>();
    for (auto ent : charView) {
      auto node = _registry->reverseLookup(ent);
      charNodeLambda(node);
    }
  }
  else {
    for (auto entity : _rigidObserver) {
      auto node = _registry->reverseLookup(entity);
      rigidNodeLambda(node);
    }

    for (auto entity : _charObserver) {
      auto node = _registry->reverseLookup(entity);
      charNodeLambda(node);
    }

    for (auto entity : _colliderObserver) {
      auto node = _registry->reverseLookup(entity);
      checkIfCreate(node);

      if (!_joltImpl->isShapeKnown(node)) return;

      // Check for all colliders
      if (_registry->hasComponent<component::MeshCollider>(node)) {
      }
      else if (_registry->hasComponent<component::SphereCollider>(node)) {
        float radius = _registry->getComponent<component::SphereCollider>(node)._radius;
        _joltImpl->updateSphere(radius, node);
      }
      else if (_registry->hasComponent<component::BoxCollider>(node)) {
        auto halfExtent = _registry->getComponent<component::BoxCollider>(node)._halfExtent;
        _joltImpl->updateBox(halfExtent, node);
      }
      else if (_registry->hasComponent<component::CapsuleCollider>(node)) {
        auto halfHeight = _registry->getComponent<component::CapsuleCollider>(node)._halfHeight;
        auto radius = _registry->getComponent<component::CapsuleCollider>(node)._radius;
        _joltImpl->updateCapsule(halfHeight, radius, node);
      }
    }
  }

  // Go through paging
  for (auto entity : _pagingObserver) {
    auto node = _registry->reverseLookup(entity);
    bool paged = _registry->getComponent<component::PageStatus>(node)._paged;

    // If paged, check if we should create a physics representation again
    if (paged) {
      checkIfCreate(node);
    }
    // Not paged, remove from physics world
    else {
      _joltImpl->remove(node);
    }
  }

  // Step the physics simulation
  if (_simulationRunning) {
    debugUpdateCharacters(delta);

    syncCharacters();

    _joltImpl->preUpdate(delta);
    _joltImpl->update(delta);
    _joltImpl->postUpdate(delta);
  }

  if (debugDraw) {
    _joltImpl->debugDraw(_debugRenderer);
  }

  _rigidObserver.clear();
  _charObserver.clear();
  _colliderObserver.clear();
  _pagingObserver.clear();
}

void PhysicsSystem::upstreamTransformSync()
{
  if (!_simulationRunning) return;

  // Sync transforms back to hierarchy
  auto syncInfos = _joltImpl->retrieveCurrentTransforms();
  for (auto& syncInfo : syncInfos) {
    // Update the local transform using the new global transform, and (if we have a parent),
    // the parent's local transform since that should be up to date.
    // If we don't have a parent we can just directly set the local transform.
    auto* nodeP = _scene->getNode(syncInfo._node);
    auto& transComp = _registry->getComponent<component::Transform>(syncInfo._node);

    // Store scale
    auto scale = util::getScale(transComp._globalTransform);

    // Calc new global trans
    auto newGlobal = syncInfo._global * glm::scale(glm::mat4(1.0f), scale);

    if (!nodeP->_parent) {
      transComp._localTransform = std::move(newGlobal);
      _registry->patchComponent<component::Transform>(syncInfo._node);
      continue;
    }

    // If we have a parent, get its inverted local transform.
    auto invParentLocal = glm::inverse(_registry->getComponent<component::Transform>(nodeP->_parent)._localTransform);

    // Apply the new local transform.
    transComp._localTransform = invParentLocal * newGlobal;

    // Let scene know
    _registry->patchComponent<component::Transform>(syncInfo._node);
  }
}

void PhysicsSystem::debugSphere()
{
  _joltImpl->debugSphere();
}

void PhysicsSystem::connectObserver()
{
  // To be able to create aswell as reflect the component changes to the physics impl.
  _rigidObserver.connect(_registry->getEnttRegistry(), 
    entt::collector.
    update<component::RigidBody>()
  );

  // To be able to create and change a character controller
  _charObserver.connect(_registry->getEnttRegistry(),
    entt::collector.
    update<component::CharacterController>()
  );

  // This is mainly for editing, to sync changes made in editor to physics.
  _transformObserver.connect(_registry->getEnttRegistry(),
    entt::collector.
    update<component::Transform>().where<component::RigidBody, component::MeshCollider>().
    update<component::Transform>().where<component::RigidBody, component::BoxCollider>().
    update<component::Transform>().where<component::RigidBody, component::SphereCollider>().
    update<component::Transform>().where<component::RigidBody, component::CapsuleCollider>().
    update<component::Transform>().where<component::CharacterController, component::BoxCollider>().
    update<component::Transform>().where<component::CharacterController, component::SphereCollider>().
    update<component::Transform>().where<component::CharacterController, component::CapsuleCollider>()
  );

  // To reflect changes to physics impl.
  _colliderObserver.connect(_registry->getEnttRegistry(),
    entt::collector.
    update<component::MeshCollider>().
    update<component::BoxCollider>().
    update<component::SphereCollider>().
    update<component::CapsuleCollider>()
  );

  // To detect if something is paged out or in.
  _pagingObserver.connect(_registry->getEnttRegistry(),
    entt::collector.
    update<component::PageStatus>().where<component::RigidBody>().
    update<component::PageStatus>().where<component::CharacterController>()
  );

  _registry->getEnttRegistry().on_destroy<component::RigidBody>().connect<&PhysicsSystem::onRemoved>(this);
  _registry->getEnttRegistry().on_destroy<component::CharacterController>().connect<&PhysicsSystem::onRemoved>(this);
}

void PhysicsSystem::checkIfCreate(const util::Uuid& node)
{
  auto& transComp = _registry->getComponent<component::Transform>(node);
  auto& globalTrans = transComp._globalTransform;
  bool hasRigidComp = _registry->hasComponent<component::RigidBody>(node);
  bool hasCharComp = _registry->hasComponent<component::CharacterController>(node);

  // Check for all colliders
  if (_registry->hasComponent<component::MeshCollider>(node)) {
    if (!_joltImpl->isShapeKnown(node)) {
      if (_registry->hasComponent<component::Renderable>(node)) {
        // For now just take the last mesh...? TODO

        auto& rendComp = _registry->getComponent<component::Renderable>(node);
        auto* model = _scene->getModel(rendComp._model);

        _joltImpl->addMesh(model->_meshes.back(), node);
      }
    }
  }
  else if (_registry->hasComponent<component::SphereCollider>(node)) {
    if (!_joltImpl->isShapeKnown(node)) {
      float radius = _registry->getComponent<component::SphereCollider>(node)._radius;
      _joltImpl->addSphere(radius, node);
    }
  }
  else if (_registry->hasComponent<component::BoxCollider>(node)) {
    if (!_joltImpl->isShapeKnown(node)) {
      auto halfExtent = _registry->getComponent<component::BoxCollider>(node)._halfExtent;
      _joltImpl->addBox(halfExtent, node);
    }
  }
  else if (_registry->hasComponent<component::CapsuleCollider>(node)) {
    if (!_joltImpl->isShapeKnown(node)) {
      auto halfHeight = _registry->getComponent<component::CapsuleCollider>(node)._halfHeight;
      auto radius = _registry->getComponent<component::CapsuleCollider>(node)._radius;
      _joltImpl->addCapsule(halfHeight, radius, node);
    }
  }

  // Check if rigidbody is known, do last since it needs a shape
  if (!_joltImpl->isBodyKnown(node) && hasRigidComp) {
    auto& rigidComp = _registry->getComponent<component::RigidBody>(node);
    _joltImpl->addRigidBody(rigidComp, globalTrans, node);
  }

  // Same for character
  if (!_joltImpl->isCharKnown(node) && hasCharComp) {
    auto& charComp = _registry->getComponent<component::CharacterController>(node);
    _joltImpl->addCharacterController(charComp, globalTrans, node);
  }
}

void PhysicsSystem::onRemoved(entt::registry& reg, entt::entity entity)
{
  auto node = _registry->reverseLookup(entity);
  if (!_joltImpl->isBodyKnown(node) && 
    !_joltImpl->isCharKnown(node) && 
    !_joltImpl->isShapeKnown(node)) return;

  _joltImpl->remove(node);
}

void PhysicsSystem::syncCharacters()
{
  auto view = _registry->getEnttRegistry().view<component::CharacterController, component::PageStatus>();

  for (auto ent : view) {
    auto node = _registry->reverseLookup(ent);
    auto& charComp = _registry->getComponent<component::CharacterController>(node);
    auto& pageComp = _registry->getComponent<component::PageStatus>(node);

    if (!pageComp._paged) continue;

    _joltImpl->setDesiredVelocity(charComp._desiredLinearVelocity, charComp._jumpSpeed, charComp._speed, node);
  }
}

void PhysicsSystem::debugUpdateCharacters(double delta)
{
  auto view = _registry->getEnttRegistry().view<component::CharacterController>();

  for (auto ent : view) {
    auto node = _registry->reverseLookup(ent);

    auto& charComp = _registry->getComponent<component::CharacterController>(node);

    charComp._desiredLinearVelocity = glm::vec3(1.0f, 0.0f, 0.0f);
  }
}

}