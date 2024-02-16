#pragma once

#include <entt/entt.hpp>

#include "../util/Uuid.h"

namespace component {
  class Registry;
}

namespace render {
  class RenderContext;
}

namespace render::scene {
  class Scene;
}

namespace physics {

class JoltDebugRenderer;
class PhysicsJoltImpl;

class PhysicsSystem
{
public:
  PhysicsSystem(component::Registry* registry, render::scene::Scene* scene, render::RenderContext* rc);
  ~PhysicsSystem();

  void setRegistry(component::Registry* registry);
  void setScene(render::scene::Scene* scene);

  PhysicsSystem(const PhysicsSystem&) = delete;
  PhysicsSystem& operator=(const PhysicsSystem&) = delete;
  PhysicsSystem(PhysicsSystem&&) = delete;
  PhysicsSystem& operator=(PhysicsSystem&&) = delete;

  void init();

  // Sync transforms from hierarchy to physics simulation.
  void downstreamTransformSync();

  // Creates bodies and colliders, and steps the simulation if it is currently running.
  void update(double delta, bool debugDraw = true);

  // Sync transforms from physics simulation to hierarchy.
  void upstreamTransformSync();

  bool& simulationRunning() { return _simulationRunning; }

  void debugSphere();

private:
  void connectObserver();
  void checkIfCreate(const util::Uuid& node);

  void onRemoved(entt::registry& reg, entt::entity entity);

  void syncCharacters();

  // Debugging, acts as an AI script
  void debugUpdateCharacters(double delta);

  PhysicsJoltImpl* _joltImpl = nullptr;
  JoltDebugRenderer* _debugRenderer = nullptr;
  component::Registry* _registry = nullptr;
  render::scene::Scene* _scene = nullptr;
  render::RenderContext* _rc = nullptr;

  entt::observer _rigidObserver;
  entt::observer _charObserver;
  entt::observer _transformObserver;
  entt::observer _colliderObserver;
  entt::observer _pagingObserver;
  bool _goThroughEverything = false;
  bool _simulationRunning = false;
};

}