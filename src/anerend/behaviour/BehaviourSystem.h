#pragma once

#include <entt/entt.hpp>

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

namespace render::scene { class Scene; }
namespace component { class Registry; }
namespace util { class Uuid; }

namespace behaviour {

class IBehaviour;

typedef std::function<IBehaviour* ()> BehaviourCreateFcn;

class BehaviourSystem
{
public:
  BehaviourSystem() = default;
  ~BehaviourSystem();

  BehaviourSystem(const BehaviourSystem&) = delete;
  BehaviourSystem(BehaviourSystem&&) = delete;
  BehaviourSystem& operator=(BehaviourSystem&&) = delete;
  BehaviourSystem& operator=(const BehaviourSystem&) = delete;

  // The create function should dynamically allocate the behaviour. The System will delete it when it is done.
  void registerBehaviour(const std::string& name, BehaviourCreateFcn createFcn);

  void update(double delta);

  void setRegistry(component::Registry* registry);
  void setScene(render::scene::Scene* scene);

private:
  component::Registry* _registry = nullptr;
  render::scene::Scene* _scene = nullptr;

  // All currently registered behaviours. The string corresponds to the name set in the behaviour component.
  std::unordered_map<std::string, BehaviourCreateFcn> _behaviourFactory;

  // Current active behaviours.
  std::vector<IBehaviour*> _behaviours;

  bool isKnown(const util::Uuid& id) const;
  void onBehaviourDestroyed(entt::registry& reg, entt::entity ent);
  void onBehaviourCreated(entt::registry& reg, entt::entity ent);

  entt::observer _observer;
  bool _goThroughAll = true;
};

}