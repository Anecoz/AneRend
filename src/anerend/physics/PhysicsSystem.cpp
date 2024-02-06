#include "PhysicsSystem.h"

#include "../render/scene/Scene.h"

#include "../component/Components.h"
#include "../component/Registry.h"

#include "PhysicsJoltImpl.h"
#include "JoltDebugRenderer.h"

namespace physics {

static PhysicsJoltImpl* g_JoltImpl = nullptr;

PhysicsSystem::PhysicsSystem(component::Registry* registry, render::scene::Scene* scene, render::RenderContext* rc)
  : _debugRenderer(nullptr)
  , _registry(registry)
  , _scene(scene)
{
  _terrainObserver.connect(_registry->getEnttRegistry(), entt::collector.update<component::Terrain>());

  g_JoltImpl = new PhysicsJoltImpl();
  g_JoltImpl->init();

  _debugRenderer = new JoltDebugRenderer(rc);
}

PhysicsSystem::~PhysicsSystem()
{
  delete g_JoltImpl;
  delete _debugRenderer;
}

void PhysicsSystem::setRegistry(component::Registry* registry)
{
  _registry = registry;
  _terrainObserver.connect(_registry->getEnttRegistry(), entt::collector.update<component::Terrain>());
  _goThroughEverything = true;
}

void PhysicsSystem::setScene(render::scene::Scene* scene)
{
  _scene = scene;
  _goThroughEverything = true;
}

void PhysicsSystem::init()
{
  _debugRenderer->init();
}

void PhysicsSystem::update(double delta, bool debugDraw)
{
  auto terrainLambda = [this](util::Uuid& node) {
    auto& terrainComp = _registry->getComponent<component::Terrain>(node);

    if (_registry->hasComponent<component::Renderable>(node) &&
      !_registry->hasComponent<component::RigidBody>(node)) {
      // Register a new heightfield shape and body
      auto& rendComp = _registry->getComponent<component::Renderable>(node);
      auto* model = _scene->getModel(rendComp._model);
      g_JoltImpl->addHeightfield(model->_meshes.back(), node);

      _registry->addComponent<component::RigidBody>(node);
    }
  };

  if (_goThroughEverything) {
    _goThroughEverything = false;

    auto terrainView = _registry->getEnttRegistry().view<component::Terrain>();
    for (auto ent : terrainView) {
      auto node = _registry->reverseLookup(ent);
      terrainLambda(node);
    }
  }
  else {
    // Any terrains to generate
    for (auto entity : _terrainObserver) {
      auto node = _registry->reverseLookup(entity);
      terrainLambda(node);
    }
  }

  // Step the physics simulation
  g_JoltImpl->update(delta);

  if (debugDraw) {
    g_JoltImpl->debugDraw(_debugRenderer);
  }

  _terrainObserver.clear();
}

void PhysicsSystem::debugSphere()
{
  g_JoltImpl->debugSphere();
}

}