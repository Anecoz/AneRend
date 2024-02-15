#include "Scene.h"

#include "internal/SceneSerializer.h"

#include <glm/gtx/matrix_decompose.hpp>

#include <algorithm>
#include <execution>

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

bool shouldBePatched(const util::Uuid& node, component::Registry& reg)
{
  if (reg.hasComponent<component::Renderable>(node)) return true;
  if (reg.hasComponent<component::Light>(node)) return true;
  if (reg.hasComponent<component::RigidBody>(node)) return true;

  return false;
}

}

Scene::Scene()
  : _transformObserver(_registry.getEnttRegistry(), entt::collector.update<component::Transform>())
  , _atomicPatchIndex(0)
{}

Scene::~Scene()
{}

Scene::Scene(Scene&& rhs)
  : _transformObserver(_registry.getEnttRegistry(), entt::collector.update<component::Transform>())
  , _atomicPatchIndex(0)
{
  rhs._transformObserver.disconnect();

  std::swap(_tiles, rhs._tiles);
  std::swap(_materials, rhs._materials);
  std::swap(_animations, rhs._animations);
  std::swap(_models, rhs._models);
  std::swap(_prefabs, rhs._prefabs);
  std::swap(_textures, rhs._textures);
  std::swap(_eventLog, rhs._eventLog);
  std::swap(_nodes, rhs._nodes);
  std::swap(_nodeVec, rhs._nodeVec);
  std::swap(_registry, rhs._registry);
  std::swap(_cinematics, rhs._cinematics);

  _transformObserver.connect(_registry.getEnttRegistry(), entt::collector.update<component::Transform>());
  _goThroughAllNodes = true;
}

Scene& Scene::operator=(Scene&& rhs)
{
  if (this != &rhs) {
    rhs._transformObserver.disconnect();

    std::swap(_tiles, rhs._tiles);
    std::swap(_materials, rhs._materials);
    std::swap(_animations, rhs._animations);
    std::swap(_models, rhs._models);
    std::swap(_prefabs, rhs._prefabs);
    std::swap(_textures, rhs._textures);
    std::swap(_eventLog, rhs._eventLog);
    std::swap(_nodes, rhs._nodes);
    std::swap(_nodeVec, rhs._nodeVec);
    std::swap(_registry, rhs._registry);
    std::swap(_cinematics, rhs._cinematics);

    _transformObserver.connect(_registry.getEnttRegistry(), entt::collector.update<component::Transform>());
  }

  _atomicPatchIndex.store(0);
  _goThroughAllNodes = true;
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
      _registry.unregisterNode(it->_node);
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

  auto lambda = [this](Node& node) {
    //updateDependentTransforms(node);

    // Add to correct tile
    auto& trans = _registry.getComponent<component::Transform>(node._id);
    auto tileIdx = findTransformTile(trans, Tile::_tileSize);

    // Is it already added to a tile?
    bool updateTile = false;
    if (_nodeTileMap.find(node._id) != _nodeTileMap.end()) {
      if (tileIdx != _nodeTileMap[node._id]) {
        updateTile = true;
        auto& oldIdx = _nodeTileMap[node._id];
        _tiles[oldIdx].removeNode(node._id);
      }
    }
    else {
      updateTile = true;
    }

    if (updateTile) {
      if (_tiles.find(tileIdx) != _tiles.end()) {
        _tiles[tileIdx].addNode(node._id);
        _tiles[tileIdx].dirty() = true;
        _nodeTileMap[node._id] = tileIdx;
      }
      else {
        Tile tile(tileIdx);
        tile.addNode(node._id);
        _tiles[tileIdx] = std::move(tile);
        _tiles[tileIdx].dirty() = true;
        _nodeTileMap[node._id] = tileIdx;
      }
    }};

  if (_goThroughAllNodes.load()) {
    for (auto& node : _nodeVec) {
      updateDependentTransforms(node);
      lambda(node);
    }

    _goThroughAllNodes.store(false);
  }
  else {
    // Try to parallelise the transform update.
    // We start by finding all roots of touched transforms, this is an expensive operation
    // but we have to do it in order to have independent updates for each root tree to run 
    // across multiple threads.
    std::vector<util::Uuid> rootsToUpdate;
    for (const auto entity : _transformObserver) {
      auto id = _registry.reverseLookup(entity);
      auto& node = _nodeVec[_nodes[id]];
      rootsToUpdate.emplace_back(findRoot(node));
    }

    // Uniqueify
    rootsToUpdate.erase(std::unique(rootsToUpdate.begin(), rootsToUpdate.end()), rootsToUpdate.end());

    // Let the implementation decide how to parallise the udpate.
    if (_nodesToBePatched.size() < 1024) {
      _nodesToBePatched.resize(1024);
    }

    std::for_each(
      std::execution::par_unseq,
      rootsToUpdate.begin(),
      rootsToUpdate.end(),
      [this](auto&& id) {
        auto& node = _nodeVec[_nodes[id]];
        updateDependentTransforms(node);
      }
    );

    // Patch stuff up that was touched and deemed necessary to update
    auto maxIdx = _atomicPatchIndex.load();
    for (std::size_t i = 0; i < maxIdx; ++i) {
      auto& id = _nodesToBePatched[i];
      auto& node = _nodeVec[_nodes[id]];

      _registry.patchComponent<component::Transform>(id);
      lambda(node);
    }
  }  

  _transformObserver.clear();
  _atomicPatchIndex.store(0);
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
  // Need to make sure that we delete any parent referencing this as a child and any child referencing this as a parent
  util::Uuid parent;
  bool found = false;

  for (auto it = _prefabs.begin(); it != _prefabs.end(); ++it) {
    if (it->_id == id) {
      found = true;
      parent = it->_parent;
      _prefabs.erase(it);
      addEvent(SceneEventType::PrefabRemoved, id);
      break;
    }
  }

  // Remove id as a child
  if (parent) {
    for (auto it = _prefabs.begin(); it != _prefabs.end(); ++it) {
      if (it->_id == parent) {
        for (auto it2 = it->_children.begin(); it2 != it->_children.end(); ++it) {
          if (*it2 == id) {
            it->_children.erase(it2);
            break;
          }
        }
        break;
      }
    }
  }

  // Make sure no one has us as a parent
  for (auto it = _prefabs.begin(); it != _prefabs.end(); ++it) {
    if (it->_parent == id) {
      it->_parent = util::Uuid();
    }
  }

  if (!found) {
    printf("Could not remove prefab %s, doesn't exist!\n", id.str().c_str());
  }
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

void Scene::updateTexture(asset::Texture texture)
{
  for (auto it = _textures.begin(); it != _textures.end(); ++it) {
    if (it->_id == texture._id) {
      *it = std::move(texture);
      addEvent(SceneEventType::TextureUpdated, it->_id);
      return;
    }
  }

  printf("Could not update texture with id %s, doesn't exist!\n", texture._id.str().c_str());
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

anim::Animation* Scene::getAnimationMut(util::Uuid id)
{
  anim::Animation* animation = nullptr;
  getAsset(_animations, id, &animation);
  return animation;
}

util::Uuid Scene::addCinematic(asset::Cinematic cinematic)
{
  auto id = cinematic._id;
  _cinematics.emplace_back(std::move(cinematic));
  return id;
}

void Scene::updateCinematic(asset::Cinematic cinematic)
{
  for (auto it = _cinematics.begin(); it != _cinematics.end(); ++it) {
    if (it->_id == cinematic._id) {
      *it = cinematic;
      return;
    }
  }

  printf("Could not update cinematic with id %s, doesn't exist!\n", cinematic._id.str().c_str());
}

void Scene::removeCinematic(util::Uuid id)
{
  for (auto it = _cinematics.begin(); it != _cinematics.end(); ++it) {
    if (it->_id == id) {
      _cinematics.erase(it);
      return;
    }
  }

  printf("Could not remove cinematic with id %s, doesn't exist!\n", id.str().c_str());
}

const asset::Cinematic* Scene::getCinematic(util::Uuid id)
{
  asset::Cinematic* cinematic = nullptr;
  getAsset(_cinematics, id, &cinematic);
  return cinematic;
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

void Scene::setNodeAsChild(util::Uuid& node, util::Uuid& child)
{
  // First we have to unset child as a child in parent, if there was one
  if (_nodes.find(node) == _nodes.end() || _nodes.find(child) == _nodes.end()) {
    assert("Nodes don't exist!");
    return;
  }

  auto& childNode = _nodeVec[_nodes[child]];
  if (childNode._parent) {
    auto& oldParentNode = _nodeVec[_nodes[childNode._parent]];
    for (auto it = oldParentNode._children.begin(); it != oldParentNode._children.end(); ++it){
      if (*it == child) {
        oldParentNode._children.erase(it);
        break;
      }
    }
  }

  auto& nodeRef = _nodeVec[_nodes[node]];
  childNode._parent = node;
  nodeRef._children.emplace_back(child);

  // Touch the transform of parent to force an update
  _registry.patchComponent<component::Transform>(node);
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

  return {};
}

component::PotentialComponents Scene::nodeToPotComps(util::Uuid& node)
{
  auto& n = _nodeVec[_nodes[node]];

  component::PotentialComponents out;

  // Always transform
  out._trans = _registry.getComponent<component::Transform>(node);

  if (_registry.hasComponent<component::Renderable>(node)) {
    out._rend = _registry.getComponent<component::Renderable>(node);
  }
  if (_registry.hasComponent<component::Light>(node)) {
    out._light = _registry.getComponent<component::Light>(node);
  }
  if (_registry.hasComponent<component::Skeleton>(node)) {
    out._skeleton = _registry.getComponent<component::Skeleton>(node);
  }
  if (_registry.hasComponent<component::Animator>(node)) {
    out._animator = _registry.getComponent<component::Animator>(node);
  }
  if (_registry.hasComponent<component::Terrain>(node)) {
    out._terrain = _registry.getComponent<component::Terrain>(node);
  }
  if (_registry.hasComponent<component::RigidBody>(node)) {
    out._rigidBody = _registry.getComponent<component::RigidBody>(node);
  }
  if (_registry.hasComponent<component::SphereCollider>(node)) {
    out._sphereColl = _registry.getComponent<component::SphereCollider>(node);
  }
  if (_registry.hasComponent<component::MeshCollider>(node)) {
    out._meshColl = _registry.getComponent<component::MeshCollider>(node);
  }
  if (_registry.hasComponent<component::BoxCollider>(node)) {
    out._boxColl = _registry.getComponent<component::BoxCollider>(node);
  }
  if (_registry.hasComponent<component::CapsuleCollider>(node)) {
    out._capsuleColl = _registry.getComponent<component::CapsuleCollider>(node);
  }
  if (_registry.hasComponent<component::CharacterController>(node)) {
    out._charCon = _registry.getComponent<component::CharacterController>(node);
  }

  return out;
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
    if (!_goThroughAllNodes && shouldBePatched(childId, _registry)) {
      auto idx = _atomicPatchIndex.fetch_add(1, std::memory_order_relaxed);
      //printf("Got idx %u\n", idx);
      _nodesToBePatched[idx] = childId;
    }
    else if (_goThroughAllNodes) {
      _registry.patchComponent<component::Transform>(childId);
    }

    if (!child._children.empty()) {
      updateChildrenTransforms(child);
    }
  }
}

util::Uuid Scene::findRoot(const Node& node)
{
  if (!node._parent) {
    return node._id;
  }

  auto& parNod = _nodeVec[_nodes[node._parent]];
  return findRoot(parNod);
}

void Scene::updateDependentTransforms(Node& node)
{
  glm::mat4 parentGlobal(1.0f);
  if (node._parent) {
    parentGlobal = _registry.getComponent<component::Transform>(node._parent)._globalTransform;
  }

  auto& nodeTrans = _registry.getComponent<component::Transform>(node._id);
  nodeTrans._globalTransform = parentGlobal * nodeTrans._localTransform;
  if (!_goThroughAllNodes && shouldBePatched(node._id, _registry)) {
    auto idx = _atomicPatchIndex.fetch_add(1, std::memory_order_relaxed);
    _nodesToBePatched[idx] = node._id;
  }
  else if (_goThroughAllNodes) {
    _registry.patchComponent<component::Transform>(node._id);
  }

  if (node._children.empty()) {
    return;
  }

  // Will recursively take care of all children
  updateChildrenTransforms(node);
}

}