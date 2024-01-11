#include "Scene.h"

#include "internal/SceneSerializer.h"

#include <glm/gtx/matrix_decompose.hpp>

namespace render::scene {

namespace {

TileIndex findTransformTile(const component::Transform& transform, unsigned tileSize)
{
  glm::vec3 t = transform._globalTransform[3];
  return Tile::posToIdx(t);
}

template<typename T, typename IDType>
bool getAsset(std::vector<T>& vec, IDType id, T** tOut)
{
  for (auto& t : vec) {
    if (t._id == id) {
      *tOut = &t;
      return true;
    }
  }
  return false;
}

}

Scene::Scene()
  : _transformObserver(_registry.getEnttRegistry(), entt::collector.update<component::Transform>())
{}

Scene::~Scene()
{}

Scene::Scene(Scene&& rhs)
  : _transformObserver(_registry.getEnttRegistry(), entt::collector.update<component::Transform>())
{
  std::swap(_tiles, rhs._tiles);
  std::swap(_materials, rhs._materials);
  std::swap(_animations, rhs._animations);
  std::swap(_skeletons, rhs._skeletons);
  std::swap(_models, rhs._models);
  std::swap(_animators, rhs._animators);
  std::swap(_prefabs, rhs._prefabs);
  std::swap(_textures, rhs._textures);
  std::swap(_eventLog, rhs._eventLog);
  std::swap(_nodes, rhs._nodes);
  std::swap(_nodeVec, rhs._nodeVec);
}

Scene& Scene::operator=(Scene&& rhs)
{
  if (this != &rhs) {
    std::swap(_tiles, rhs._tiles);
    std::swap(_materials, rhs._materials);
    std::swap(_animations, rhs._animations);
    std::swap(_skeletons, rhs._skeletons);
    std::swap(_models, rhs._models);
    std::swap(_animators, rhs._animators);
    std::swap(_prefabs, rhs._prefabs);
    std::swap(_textures, rhs._textures);
    std::swap(_eventLog, rhs._eventLog);
    std::swap(_nodes, rhs._nodes);
    std::swap(_nodeVec, rhs._nodeVec);
  }

  return *this;
}

void Scene::update()
{
  // Update state of nodes pending removal
  for (auto it = _nodesPendingRemoval.begin(); it != _nodesPendingRemoval.end();) {
    if (it->_counter >= 1) {
      // Remove any hanging parent-child relationship
      auto& node = _nodeVec[_nodes[it->_node]];
      if (node._parent && _nodes.find(node._parent) != _nodes.end()) {
        auto& pChildVec = _nodeVec[_nodes[node._parent]]._children;
        for (auto it = pChildVec.begin(); it != pChildVec.end(); ++it) {
          if ((*it) == node._id) {
            pChildVec.erase(it);
            break;
          }
        }
      }

      // Everything should have had the chance to page out and disassociate itself with the node by now (one full update cycle)
      _nodes.erase(it->_node);
      _nodeVec.erase(std::remove_if(_nodeVec.begin(), _nodeVec.end(), [id = it->_node](auto& a) {return a._id == id; }), _nodeVec.end());
      _nodeTileMap.erase(it->_node);

      // Update map (TODO: Make a static array?)
      _nodes.clear();
      for (std::size_t i = 0; i < _nodeVec.size(); ++i) {
        _nodes[_nodeVec[i]._id] = i;
      }

      it = _nodesPendingRemoval.erase(it);
    }
    else {
      // Remove from tiles so that they can page out
      auto& tileIndex = _nodeTileMap[it->_node];
      _tiles[tileIndex].removeNode(it->_node);

      it->_counter++;
      it++;
    }
  }

  // Check if any transforms have been updated
  for (const auto entity : _transformObserver) {

    // Update the transform hierarchy
    auto id = _registry.reverseLookup(entity);
    auto& node = _nodeVec[_nodes[id]];
    updateDependentTransforms(node);

    // Add to correct tile
    auto& trans = _registry.getEnttRegistry().get<component::Transform>(entity);
    auto tileIdx = findTransformTile(trans, Tile::_tileSize);

    // Is it already added to a tile?
    if (_nodeTileMap.find(id) != _nodeTileMap.end()) {
      auto& oldIdx = _nodeTileMap[id];
      _tiles[oldIdx].removeNode(id);
    }

    if (_tiles.find(tileIdx) != _tiles.end()) {
      _tiles[tileIdx].addNode(id);
      _tiles[tileIdx].dirty() = true;
      _nodeTileMap[id] = tileIdx;
    }
    else {
      Tile tile(tileIdx);
      tile.addNode(id);
      _tiles[tileIdx] = std::move(tile);
      _tiles[tileIdx].dirty() = true;
      _nodeTileMap[id] = tileIdx;
    }
  }

  _transformObserver.clear();
}

void Scene::serializeAsync(const std::filesystem::path& path)
{
  _serialiser.serialize(*this, path);
}

std::future<DeserialisedSceneData> Scene::deserializeAsync(const std::filesystem::path& path)
{
  return _serialiser.deserialize(path);
}

const SceneEventLog& Scene::getEvents() const
{
  return _eventLog;
}

void Scene::resetEvents()
{
  _eventLog._events.clear();
}

bool Scene::getTile(TileIndex idx, Tile** tileOut)
{
  if (_tiles.count(idx) == 0) {
    return false;
  }

  *tileOut = &_tiles[idx];
  return true;
}

util::Uuid Scene::addPrefab(asset::Prefab&& prefab)
{
  auto id = prefab._id;
  addEvent(SceneEventType::PrefabAdded, prefab._id);
  _prefabs.emplace_back(std::move(prefab));
  return id;
}

void Scene::updatePrefab(asset::Prefab prefab)
{
  for (auto it = _prefabs.begin(); it != _prefabs.end(); ++it) {
    if (it->_id == prefab._id) {
      *it = prefab;
      addEvent(SceneEventType::PrefabUpdated, prefab._id);
      return;
    }
  }

  printf("Could not update prefab with id %s, doesn't exist!\n", prefab._id.str().c_str());
}

void Scene::removePrefab(util::Uuid id)
{
  for (auto it = _prefabs.begin(); it != _prefabs.end(); ++it) {
    if (it->_id == id) {
      _prefabs.erase(it);
      addEvent(SceneEventType::PrefabRemoved, id);
      return;
    }
  }

  printf("Could not remove model %s, doesn't exist!\n", id.str().c_str());
}

const asset::Prefab* Scene::getPrefab(util::Uuid id)
{
  asset::Prefab* p = nullptr;
  getAsset(_prefabs, id, &p);
  return p;
}

util::Uuid Scene::addTexture(asset::Texture&& texture)
{
  auto id = texture._id;
  addEvent(SceneEventType::TextureAdded, texture._id);
  _textures.emplace_back(std::move(texture));
  return id;
}

void Scene::removeTexture(util::Uuid id)
{
  for (auto it = _textures.begin(); it != _textures.end(); ++it) {
    if (it->_id == id) {
      _textures.erase(it);
      addEvent(SceneEventType::TextureRemoved, id);
      return;
    }
  }

  printf("Could not remove model %s, doesn't exist!\n", id.str().c_str());
}

const asset::Texture* Scene::getTexture(util::Uuid id)
{
  asset::Texture* p = nullptr;
  getAsset(_textures, id, &p);
  return p;
}

util::Uuid Scene::addModel(asset::Model&& model)
{
  auto id = model._id;

  addEvent(SceneEventType::ModelAdded, model._id);
  _models.emplace_back(std::move(model));
  return id;
}

void Scene::removeModel(util::Uuid id)
{
  for (auto it = _models.begin(); it != _models.end(); ++it) {
    if (it->_id == id) {
      _models.erase(it);
      addEvent(SceneEventType::ModelRemoved, id);
      return;
    }
  }

  printf("Could not remove model %s, doesn't exist!\n", id.str().c_str());
}

const asset::Model* Scene::getModel(util::Uuid id)
{
  asset::Model* model = nullptr;
  getAsset(_models, id, &model);
  return model;
}

util::Uuid Scene::addMaterial(asset::Material&& material)
{
  auto id = material._id;
  addEvent(SceneEventType::MaterialAdded, material._id);
  _materials.emplace_back(std::move(material));
  return id;
}

void Scene::updateMaterial(asset::Material material)
{
  for (auto it = _materials.begin(); it != _materials.end(); ++it) {
    if (it->_id == material._id) {
      *it = material;
      addEvent(SceneEventType::MaterialUpdated, material._id);
      return;
    }
  }

  printf("Could not update material with id %s, doesn't exist!\n", material._id.str().c_str());
}

void Scene::removeMaterial(util::Uuid id)
{
  for (auto it = _materials.begin(); it != _materials.end(); ++it) {
    if (it->_id == id) {
      _materials.erase(it);
      addEvent(SceneEventType::MaterialRemoved, id);
      return;
    }
  }

  printf("Could not remove material with id %s, doesn't exist!\n", id.str().c_str());
}

const asset::Material* Scene::getMaterial(util::Uuid id)
{
  asset::Material* material = nullptr;
  getAsset(_materials, id, &material);
  return material;
}

util::Uuid Scene::addAnimation(anim::Animation&& animation)
{
  auto id = animation._id;
  addEvent(SceneEventType::AnimationAdded, animation._id);
  _animations.emplace_back(std::move(animation));
  return id;
}

void Scene::removeAnimation(util::Uuid id)
{
  for (auto it = _animations.begin(); it != _animations.end(); ++it) {
    if (it->_id == id) {
      _animations.erase(it);
      addEvent(SceneEventType::AnimationRemoved, id);
      return;
    }
  }

  printf("Could not remove animation %s, doesn't exist!\n", id.str().c_str());
}

const anim::Animation* Scene::getAnimation(util::Uuid id)
{
  anim::Animation* animation = nullptr;
  getAsset(_animations, id, &animation);
  return animation;
}

util::Uuid Scene::addSkeleton(anim::Skeleton&& skeleton)
{
  auto id = skeleton._id;
  addEvent(SceneEventType::SkeletonAdded, skeleton._id);
  _skeletons.emplace_back(std::move(skeleton));
  return id;
}

void Scene::removeSkeleton(util::Uuid id)
{
  for (auto it = _skeletons.begin(); it != _skeletons.end(); ++it) {
    if (it->_id == id) {
      _skeletons.erase(it);
      addEvent(SceneEventType::SkeletonRemoved, id);
      return;
    }
  }

  printf("Could not remove skeleton with id %s, doesn't exist!\n", id.str().c_str());
}

const anim::Skeleton* Scene::getSkeleton(util::Uuid id)
{
  anim::Skeleton* skele = nullptr;
  getAsset(_skeletons, id, &skele);
  return skele;
}

util::Uuid Scene::addAnimator(asset::Animator&& animator)
{
  auto id = animator._id;
  addEvent(SceneEventType::AnimatorAdded, animator._id);
  _animators.emplace_back(std::move(animator));
  return id;
}

void Scene::updateAnimator(asset::Animator animator)
{
  for (auto it = _animators.begin(); it != _animators.end(); ++it) {
    if (it->_id == animator._id) {
      *it = animator;
      addEvent(SceneEventType::AnimatorUpdated, animator._id);
      return;
    }
  }

  printf("Could not update animator with id %s, doesn't exist!\n", animator._id.str().c_str());
}

void Scene::removeAnimator(util::Uuid id)
{
  for (auto it = _animators.begin(); it != _animators.end(); ++it) {
    if (it->_id == id) {
      _animators.erase(it);
      addEvent(SceneEventType::AnimatorRemoved, id);
      return;
    }
  }

  printf("Could not remove animator with id %s, doesn't exist!\n", id.str().c_str());
}

const asset::Animator* Scene::getAnimator(util::Uuid id)
{
  asset::Animator* animator = nullptr;
  getAsset(_animators, id, &animator);
  return animator;
}

util::Uuid Scene::addNode(Node node)
{
  auto id = node._id;
  _registry.registerNode(id);
  _nodeVec.emplace_back(node);
  _nodes[id] = _nodeVec.size() - 1;
  return id;
}

void Scene::removeNode(util::Uuid id)
{
  if (_nodes.find(id) != _nodes.end()) {
    auto& node = _nodeVec[_nodes[id]];
    _nodesPendingRemoval.emplace_back(id, 0);

    for (auto& childId : node._children) {
      removeNode(childId);
    }
  }
}

const Node* Scene::getNode(util::Uuid id)
{
  if (_nodes.find(id) != _nodes.end()) {
    return &_nodeVec[_nodes[id]];
  }

  return nullptr;
}

void Scene::setNodeName(util::Uuid& node, std::string name)
{
  if (_nodes.find(node) != _nodes.end()) {
    _nodeVec[_nodes[node]]._name = std::move(name);
  }
}

void Scene::addNodeChild(util::Uuid& node, util::Uuid& child)
{
  // UB if child was already added as a child to another node
  if (_nodes.find(node) != _nodes.end()) {
    if (_nodes.find(child) != _nodes.end()) {
      assert(!_nodeVec[_nodes[child]]._parent && "Cannot set node to child, it already has a parent!");

      _nodeVec[_nodes[node]]._children.emplace_back(child);
      _nodeVec[_nodes[child]]._parent = node;
    }
  }
}

void Scene::removeNodeChild(util::Uuid& node, util::Uuid& child)
{
  if (_nodes.find(node) != _nodes.end()) {
    if (_nodes.find(child) != _nodes.end()) {
      assert(_nodeVec[_nodes[child]]._parent == node && "Cannot remove parent from node, ids don't match!");

      auto& childVec = _nodeVec[_nodes[node]]._children;

      childVec.erase(std::remove(childVec.begin(), childVec.end(), child), childVec.end());
      _nodeVec[_nodes[child]]._parent = util::Uuid();
    }
  }
}

std::vector<util::Uuid> Scene::getNodeChildren(util::Uuid& node)
{
  if (_nodes.find(node) != _nodes.end()) {
    return _nodeVec[_nodes[node]]._children;
  }
}

void Scene::setDDGIAtlas(util::Uuid texId, scene::TileIndex idx)
{
  if (_tiles.find(idx) != _tiles.end()) {
    _tiles[idx].setDDGIAtlas(texId);
  }
  else {
    // Add a new tile here
    Tile tile(idx);
    tile.setDDGIAtlas(texId);
    _tiles[idx] = std::move(tile);
  }

  // add to tile infos (could be more in here in the future)
  bool found = false;
  for (auto& ti : _tileInfos) {
    if (ti._idx == idx) {
      ti._ddgiAtlas = texId;
      found = true;
      break;
    }
  }

  if (!found) {
    TileInfo ti{};
    ti._ddgiAtlas = texId;
    ti._idx = idx;
    _tileInfos.emplace_back(std::move(ti));
  }

  addEvent(SceneEventType::DDGIAtlasAdded, util::Uuid(), idx);
}

void Scene::addEvent(SceneEventType type, util::Uuid id, TileIndex tileIdx)
{
  SceneEvent event{};
  event._id = id;
  event._type = type;
  event._tileIdx = tileIdx;
  _eventLog._events.emplace_back(std::move(event));
}

void Scene::updateChildrenTransforms(Node& node)
{
  auto& nodeTrans = _registry.getComponent<component::Transform>(node._id);
  for (auto& childId : node._children) {
    auto& child = _nodeVec[_nodes[childId]];
    auto& childTransform = _registry.getComponent<component::Transform>(childId);

    childTransform._globalTransform = nodeTrans._globalTransform * childTransform._localTransform;
    _registry.patchComponent<component::Transform>(childId);

    if (!child._children.empty()) {
      updateChildrenTransforms(child);
    }
  }
}

void Scene::updateDependentTransforms(Node& node)
{
  // Figure out which renderables are involved
  if (!node._parent) {
    auto& nodeTrans = _registry.getComponent<component::Transform>(node._id);
    nodeTrans._globalTransform = nodeTrans._localTransform;
    _registry.patchComponent<component::Transform>(node._id);

    if (node._children.empty()) {
      return;
    }

    // Will recursively take care of all children
    updateChildrenTransforms(node);
  }
  else {
    auto& parentTrans = _registry.getComponent<component::Transform>(node._parent);
    updateDependentTransforms(_nodeVec[_nodes[node._parent]]);
    _registry.patchComponent<component::Transform>(node._parent);
  }
}

}