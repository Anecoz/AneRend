#include "BehaviourSystem.h"

#include "IBehaviour.h"
#include "BehaviourHelper.h"

#include "../component/Components.h"
#include "../component/Registry.h"

namespace behaviour {

BehaviourSystem::~BehaviourSystem()
{
  for (auto behaviour : _behaviours) {
    delete behaviour;
  }

  _behaviours.clear();
}

void BehaviourSystem::registerBehaviour(const std::string& name, BehaviourCreateFcn createFcn)
{
  _behaviourFactory[name] = std::move(createFcn);
}

void BehaviourSystem::update(double delta)
{
  // Check observer and see if we need to create a new behaviour.
  if (_goThroughAll) {
    auto view = _registry->getEnttRegistry().view<component::Behaviour>();

    for (auto ent : view) {
      auto id = _registry->reverseLookup(ent);
      if (!isKnown(id)) {
        onBehaviourCreated(_registry->getEnttRegistry(), ent);
      }
    }
    _goThroughAll = false;
  }
  else {
    for (auto ent : _observer) {
      auto id = _registry->reverseLookup(ent);
      if (!isKnown(id)) {
        onBehaviourCreated(_registry->getEnttRegistry(), ent);
      }
    }
  }

  // Go through and update the active behaviours.
  for (auto behaviour : _behaviours) {
    behaviour->update(delta);
  }

  _observer.clear();
}

void BehaviourSystem::setRegistry(component::Registry* registry)
{
  _registry = registry;

  _observer.connect(_registry->getEnttRegistry(),
    entt::collector.
    update<component::Behaviour>()
  );

  _registry->getEnttRegistry().on_destroy<component::Behaviour>().connect<&BehaviourSystem::onBehaviourDestroyed>(this);
  _registry->getEnttRegistry().on_construct<component::Behaviour>().connect<&BehaviourSystem::onBehaviourCreated>(this);

  _goThroughAll = true;
}

void BehaviourSystem::setScene(render::scene::Scene* scene)
{
  _scene = scene;

  for (auto behaviour : _behaviours) {
    BehaviourHelper::setScene(behaviour, scene);
  }
}

bool BehaviourSystem::isKnown(const util::Uuid& id) const
{
  auto it = std::find_if(_behaviours.begin(), _behaviours.end(), [&id](IBehaviour* behaviour) {
    return behaviour->me() == id;
  });

  return it != _behaviours.end();
}

void BehaviourSystem::onBehaviourDestroyed(entt::registry& reg, entt::entity ent)
{
  auto id = _registry->reverseLookup(ent);
  auto it = std::find_if(_behaviours.begin(), _behaviours.end(), [&id](IBehaviour* behaviour) {
    return behaviour->me() == id;
  });

  if (it != _behaviours.end()) {
    delete *it;
    _behaviours.erase(it);
  }
}

void BehaviourSystem::onBehaviourCreated(entt::registry& reg, entt::entity ent)
{
  auto id = _registry->reverseLookup(ent);
  if (!isKnown(id)) {
    // Create a new behaviour and start it.
    auto& behComp = _registry->getComponent<component::Behaviour>(id);

    if (!_behaviourFactory.contains(behComp._name)) {
      printf("Could not find behaviour with name %s!\n", behComp._name.c_str());
      return;
    }

    auto behaviour = _behaviourFactory[behComp._name]();
    BehaviourHelper::setMe(behaviour, id);
    BehaviourHelper::setScene(behaviour, _scene);

    behaviour->start();

    _behaviours.emplace_back(behaviour);
  }
}

}
