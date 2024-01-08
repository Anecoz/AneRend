#pragma once

#include "../util/Uuid.h"

// Unfortunately we have to leak this header.
// Using modules can probably solve this but I haven't gotten to it.
// Currently everything that links against anerend has to also have this header available.
#include <entt/entt.hpp>

#include <unordered_map>

namespace component {

class Registry
{
public:
  Registry() = default;
  ~Registry() = default;

  // No copying or moving
  Registry(const Registry&) = delete;
  Registry(Registry&&) = delete;
  Registry& operator=(const Registry&) = delete;
  Registry& operator=(Registry&&) = delete;

  void registerNode(util::Uuid& node)
  {
    auto entity = _registry.create();
    _nodeMap[node] = entity;
    _reverseNodeMap[entity] = node;
  }

  void unregisterNode(util::Uuid& node)
  {
    if (_nodeMap.find(node) != _nodeMap.end()) {
      auto entity = _nodeMap[node];
      _registry.destroy(entity);
      _nodeMap.erase(node);
      _reverseNodeMap.erase(entity);
    }
  }

  /*template <typename T>
  void */

  template<typename T, typename... Args>
  T& addComponent(util::Uuid& node, Args&&... args)
  {
    return _registry.emplace<T>(_nodeMap[node], std::forward<Args>(args)...);
  }

  template<typename T, typename... Args>
  T& addComponent(const util::Uuid& node, Args&&... args)
  {
    return _registry.emplace<T>(_nodeMap[node], std::forward<Args>(args)...);
  }

  template<typename T>
  bool hasComponent(util::Uuid& node)
  {
    return _registry.all_of<T>(_nodeMap[node]);
  }

  template<typename T>
  bool hasComponent(const util::Uuid& node)
  {
    return _registry.all_of<T>(_nodeMap[node]);
  }

  template<typename T>
  T& getComponent(util::Uuid& node)
  {
    return _registry.get<T>(_nodeMap[node]);
  }

  template<typename T>
  T& getComponent(const util::Uuid& node)
  {
    return _registry.get<T>(_nodeMap[node]);
  }

  template<typename T>
  void removeComponent(util::Uuid& node)
  {
    _registry.remove<T>(_nodeMap[node]);
  }

  template<typename T>
  void removeComponent(const util::Uuid& node)
  {
    _registry.remove<T>(_nodeMap[node]);
  }

  template <typename T>
  void patchComponent(util::Uuid& node)
  {
    _registry.patch<T>(_nodeMap[node]);
  }

  template <typename T>
  void patchComponent(const util::Uuid& node)
  {
    _registry.patch<T>(_nodeMap[node]);
  }

  util::Uuid reverseLookup(entt::entity entity)
  {
    if (_reverseNodeMap.find(entity) != _reverseNodeMap.end()) {
      return _reverseNodeMap[entity];
    }

    return util::Uuid();
  }

  entt::registry& getEnttRegistry() { return _registry; }

private:
  entt::registry _registry;
  std::unordered_map<util::Uuid, entt::entity> _nodeMap;
  std::unordered_map<entt::entity, util::Uuid> _reverseNodeMap;
};

}