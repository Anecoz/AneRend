#include "Scene.h"

#include "internal/SceneSerializer.h"

#include <glm/gtx/matrix_decompose.hpp>

namespace render::scene
{

namespace {

TileIndex findRenderableTile(const asset::Renderable& renderable, unsigned tileSize)
{
  glm::vec3 t = renderable._transform[3];
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
{}

Scene::~Scene()
{}

Scene::Scene(Scene&& rhs)
{
  std::swap(_tiles, rhs._tiles);
  std::swap(_materials, rhs._materials);
  std::swap(_animations, rhs._animations);
  std::swap(_skeletons, rhs._skeletons);
  std::swap(_models, rhs._models);
  std::swap(_animators, rhs._animators);
  std::swap(_renderables, rhs._renderables);
  std::swap(_prefabs, rhs._prefabs);
  std::swap(_eventLog, rhs._eventLog);
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
    std::swap(_renderables, rhs._renderables);
    std::swap(_prefabs, rhs._prefabs);
    std::swap(_eventLog, rhs._eventLog);
  }

  return *this;
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

util::Uuid Scene::addRenderable(asset::Renderable&& renderable)
{
  auto id = renderable._id;

  // Find which tile this renderable belongs to
  auto tileIdx = findRenderableTile(renderable, Tile::_tileSize);
  renderable._id = id;
  _renderables.emplace_back(std::move(renderable));

  auto it = _tiles.find(tileIdx);

  if (it == _tiles.end()) {
    // Add a new tile here
    Tile tile(tileIdx);
    _tiles[tileIdx] = std::move(tile);
  }

  _tiles[tileIdx].addRenderable(id);

  addEvent(SceneEventType::RenderableAdded, id, tileIdx);

  return id;
}

void Scene::removeRenderable(util::Uuid id)
{
  for (auto it = _renderables.begin(); it != _renderables.end(); ++it) {
    if (it->_id == id) {
      auto tileIdx = findRenderableTile(*it, Tile::_tileSize);
      auto tileIt = _tiles.find(tileIdx);

      if (tileIt != _tiles.end()) {
        tileIt->second.removeRenderable(id);
      }

      _renderables.erase(it);

      addEvent(SceneEventType::RenderableRemoved, id, tileIdx);
      return;
    }
  }

  printf("Could not remove renderable %s, doesn't exist!\n", id.str().c_str());
}

const asset::Renderable* Scene::getRenderable(util::Uuid id)
{
  asset::Renderable* rend = nullptr;
  getAsset(_renderables, id, &rend);
  return rend;
}

void Scene::setRenderableTint(util::Uuid id, const glm::vec3& tint)
{
  for (auto it = _renderables.begin(); it != _renderables.end(); ++it) {
    if (it->_id == id) {
      it->_tint = tint;
      auto tileIdx = findRenderableTile(*it, Tile::_tileSize);
      addEvent(SceneEventType::RenderableUpdated, id, tileIdx);
      return;
    }
  }

  printf("Cannot set tint for renderable %s, doesn't exist!\n", id.str().c_str());
}

void Scene::setRenderableTransform(util::Uuid id, const glm::mat4& transform)
{
  for (auto it = _renderables.begin(); it != _renderables.end(); ++it) {
    if (it->_id == id) {
      it->_transform = transform;

      auto tileIdx = findRenderableTile(*it, Tile::_tileSize);
      addEvent(SceneEventType::RenderableUpdated, id, tileIdx);
      return;
    }
  }

  printf("Cannot set transform for renderable %s, doesn't exist!\n", id.str().c_str());
}

void Scene::setRenderableName(util::Uuid id, std::string name)
{
  for (auto it = _renderables.begin(); it != _renderables.end(); ++it) {
    if (it->_id == id) {
      it->_name = std::move(name);

      auto tileIdx = findRenderableTile(*it, Tile::_tileSize);
      addEvent(SceneEventType::RenderableUpdated, id, tileIdx);
      return;
    }
  }

  printf("Cannot set name for renderable %s, doesn't exist!\n", id.str().c_str());
}

void Scene::setRenderableBoundingSphere(util::Uuid id, const glm::vec4& boundingSphere)
{
  for (auto it = _renderables.begin(); it != _renderables.end(); ++it) {
    if (it->_id == id) {
      it->_boundingSphere = boundingSphere;

      auto tileIdx = findRenderableTile(*it, Tile::_tileSize);
      addEvent(SceneEventType::RenderableUpdated, id, tileIdx);
      return;
    }
  }

  printf("Cannot set bounding sphere for renderable %s, doesn't exist!\n", id.str().c_str());
}

void Scene::addEvent(SceneEventType type, util::Uuid id, TileIndex tileIdx)
{
  SceneEvent event{};
  event._id = id;
  event._type = type;
  event._tileIdx = tileIdx;
  _eventLog._events.emplace_back(std::move(event));
}

}