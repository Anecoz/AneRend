#include "Scene.h"

#include <glm/gtx/matrix_decompose.hpp>

namespace render::scene
{

namespace {

TileIndex findRenderableTile(const asset::Renderable& renderable, unsigned tileSize)
{
  glm::vec3 s;
  glm::quat q;
  glm::vec3 t;

  glm::vec3 unused_0;
  glm::vec4 unused_1;

  glm::decompose(renderable._transform, s, q, t, unused_0, unused_1);

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
  std::swap(_animations, rhs._animations);
  std::swap(_skeletons, rhs._skeletons);
  std::swap(_models, rhs._models);
  std::swap(_renderables, rhs._renderables);
}

Scene& Scene::operator=(Scene&& rhs)
{
  if (this != &rhs) {
    std::swap(_tiles, rhs._tiles);
    std::swap(_animations, rhs._animations);
    std::swap(_skeletons, rhs._skeletons);
    std::swap(_models, rhs._models);
    std::swap(_renderables, rhs._renderables);
  }

  return *this;
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

ModelId Scene::addModel(asset::Model&& model)
{
  auto id = IDGenerator::genModelId();
  model._id = id;

  for (auto& mesh : model._meshes) {
    mesh._id = IDGenerator::genMeshId();
  }

  _models.emplace_back(std::move(model));
  addEvent(SceneEventType::ModelAdded, id);
  return id;
}

void Scene::removeModel(ModelId id)
{
  for (auto it = _models.begin(); it != _models.end(); ++it) {
    if (it->_id == id) {
      _models.erase(it);
      addEvent(SceneEventType::ModelRemoved, id);
      return;
    }
  }

  printf("Could not remove model %u, doesn't exist!\n", id);
}

const asset::Model* Scene::getModel(ModelId id)
{
  asset::Model* model = nullptr;
  getAsset(_models, id, &model);
  return model;
}

MaterialId Scene::addMaterial(asset::Material&& material)
{
  auto id = IDGenerator::genMaterialId();
  material._id = id;
  _materials.emplace_back(std::move(material));
  addEvent(SceneEventType::MaterialAdded, id);
  return id;
}

void Scene::removeMaterial(MaterialId id)
{
  for (auto it = _materials.begin(); it != _materials.end(); ++it) {
    if (it->_id == id) {
      _materials.erase(it);
      addEvent(SceneEventType::MaterialRemoved, id);
      return;
    }
  }

  printf("Could not remove material with id %u, doesn't exist!\n", id);
}

const asset::Material* Scene::getMaterial(MaterialId id)
{
  asset::Material* material = nullptr;
  getAsset(_materials, id, &material);
  return material;
}

AnimationId Scene::addAnimation(anim::Animation&& animation)
{
  auto id = IDGenerator::genAnimationId();
  animation._id = id;
  _animations.emplace_back(std::move(animation));
  addEvent(SceneEventType::AnimationAdded, id);
  return id;
}

void Scene::removeAnimation(AnimationId id)
{
  for (auto it = _animations.begin(); it != _animations.end(); ++it) {
    if (it->_id == id) {
      _animations.erase(it);
      addEvent(SceneEventType::AnimationRemoved, id);
      return;
    }
  }

  printf("Could not remove animation %u, doesn't exist!\n", id);
}

const anim::Animation* Scene::getAnimation(AnimationId id)
{
  anim::Animation* animation = nullptr;
  getAsset(_animations, id, &animation);
  return animation;
}

SkeletonId Scene::addSkeleton(anim::Skeleton&& skeleton)
{
  auto id = IDGenerator::genSkeletonId();
  skeleton._id = id;
  _skeletons.emplace_back(std::move(skeleton));
  addEvent(SceneEventType::SkeletonAdded, id);
  return id;
}

void Scene::removeSkeleton(SkeletonId id)
{
  for (auto it = _skeletons.begin(); it != _skeletons.end(); ++it) {
    if (it->_id == id) {
      _skeletons.erase(it);
      addEvent(SceneEventType::SkeletonRemoved, id);
      return;
    }
  }

  printf("Could not remove skeleton with id %u, doesn't exist!\n", id);
}

const anim::Skeleton* Scene::getSkeleton(SkeletonId id)
{
  anim::Skeleton* skele = nullptr;
  getAsset(_skeletons, id, &skele);
  return skele;
}

RenderableId Scene::addRenderable(asset::Renderable&& renderable)
{
  // Find which tile this renderable belongs to
  auto id = IDGenerator::genRenderableId();
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

void Scene::removeRenderable(RenderableId id)
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

  printf("Could not remove renderable %u, doesn't exist!\n", id);
}

const asset::Renderable* Scene::getRenderable(RenderableId id)
{
  asset::Renderable* rend = nullptr;
  getAsset(_renderables, id, &rend);
  return rend;
}

void Scene::setRenderableTint(RenderableId id, const glm::vec3& tint)
{
  for (auto it = _renderables.begin(); it != _renderables.end(); ++it) {
    if (it->_id == id) {
      it->_tint = tint;
      addEvent(SceneEventType::RenderableUpdated, id);
      return;
    }
  }

  printf("Cannot set tint for renderable %u, doesn't exist!\n", id);
}

void Scene::setRenderableTransform(RenderableId id, const glm::mat4& transform)
{
  for (auto it = _renderables.begin(); it != _renderables.end(); ++it) {
    if (it->_id == id) {
      it->_transform = transform;
      addEvent(SceneEventType::RenderableUpdated, id);
      return;
    }
  }

  printf("Cannot set transform for renderable %u, doesn't exist!\n", id);
}

void Scene::addEvent(SceneEventType type, uint32_t id, TileIndex tileIdx)
{
  SceneEvent event{};
  event._id = id;
  event._type = type;
  event._tileIdx = tileIdx;
  _eventLog._events.emplace_back(std::move(event));
}

}