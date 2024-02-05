#pragma once

#include <entt/entt.hpp>

namespace component {
  class Registry;
}

namespace physics {

class PhysicsSystem
{
public:
  PhysicsSystem(component::Registry* registry);
  ~PhysicsSystem() = default;

  void setRegistry(component::Registry* registry);

  PhysicsSystem(const PhysicsSystem&) = delete;
  PhysicsSystem& operator=(const PhysicsSystem&) = delete;
  PhysicsSystem(PhysicsSystem&&) = delete;
  PhysicsSystem& operator=(PhysicsSystem&&) = delete;

  void update(double delta);

private:
  component::Registry* _registry;

  entt::observer _terrainObserver;
};

}