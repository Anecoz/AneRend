#pragma once

#include <entt/entt.hpp>

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

  void update(double delta, bool debugDraw = true);

  void debugSphere();

private:
  JoltDebugRenderer* _debugRenderer = nullptr;
  component::Registry* _registry = nullptr;
  render::scene::Scene* _scene = nullptr;

  entt::observer _terrainObserver;
  bool _goThroughEverything = false;
};

}