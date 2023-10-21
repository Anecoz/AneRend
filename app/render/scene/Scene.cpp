#include "Scene.h"

#include <glm/gtx/matrix_decompose.hpp>

namespace render::scene
{

namespace {

glm::ivec2 findRenderableTile(const asset::Renderable& renderable, unsigned tileSize)
{
  glm::vec3 s;
  glm::quat q;
  glm::vec3 t;

  glm::vec3 unused_0;
  glm::vec4 unused_1;

  glm::decompose(renderable._transform, s, q, t, unused_0, unused_1);

  // Project translation to y=0 plane
  int tileX = (int)(t.x / (float)tileSize);
  int tileY = (int)(t.z / (float)tileSize);

  return { tileX, tileY };
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
  //std::swap(_meshes, rhs._meshes);
  std::swap(_models, rhs._models);
  std::swap(_renderables, rhs._renderables);
}

Scene& Scene::operator=(Scene&& rhs)
{
  if (this != &rhs) {
    std::swap(_tiles, rhs._tiles);
    std::swap(_animations, rhs._animations);
    //std::swap(_meshes, rhs._meshes);
    std::swap(_models, rhs._models);
    std::swap(_renderables, rhs._renderables);
  }

  return *this;
}

ModelId Scene::addModel(asset::Model&& model)
{
  auto id = IDGenerator::genModelId();
  model._id = id;

  for (auto& mesh : model._meshes) {
    mesh._id = IDGenerator::genMeshId();
  }

  _models[id] = std::move(model);
  return id;
}

void Scene::removeModel(ModelId id)
{
  auto it = _models.find(id);
  if (it == _models.end()) {
    printf("Could not remove model %u, doesn't exist!\n", id);
    return;
  }

  _models.erase(it);
}

AnimationId Scene::addAnimation(anim::Animation&& animation)
{
  auto id = IDGenerator::genAnimationId();
  _animations[id] = std::move(animation);
  return id;
}

void Scene::removeAnimation(AnimationId id)
{
  auto it = _animations.find(id);
  if (it == _animations.end()) {
    printf("Could not remove animation %u, doesn't exist!\n", id);
    return;
  }

  _animations.erase(it);
}

RenderableId Scene::addRenderable(asset::Renderable&& renderable)
{
  // Find which tile this renderable belongs to
  auto id = IDGenerator::genRenderableId();
  auto tileIdx = findRenderableTile(renderable, _tileSize);
  _renderables[id] = std::move(renderable);

  auto it = _tiles.find(tileIdx);

  if (it == _tiles.end()) {
    // Add a new tile here
    Tile tile(tileIdx);
    _tiles[tileIdx] = std::move(tile);
  }

  _tiles[tileIdx].addRenderable(id);

  return id;
}

void Scene::removeRenderable(RenderableId id)
{
  auto it = _renderables.find(id);
  if (it == _renderables.end()) {
    printf("Could not remove renderable %u, doesn't exist!\n", id);
    return;
  }

  asset::Renderable& rend = it->second;
  auto tileIdx = findRenderableTile(rend, _tileSize);
  auto tileIt = _tiles.find(tileIdx);

  if (tileIt != _tiles.end()) {
    tileIt->second.removeRenderable(id);
  }

  _renderables.erase(it);
}

void Scene::setRenderableTint(RenderableId id, const glm::vec3& tint)
{
  auto it = _renderables.find(id);
  if (it == _renderables.end()) {
    printf("Cannot set tint for renderable %u, doesn't exist!\n", id);
    return;
  }

  it->second._tint = tint;
}

void Scene::setRenderableTransform(RenderableId id, const glm::mat4& transform)
{
  auto it = _renderables.find(id);
  if (it == _renderables.end()) {
    printf("Cannot set transform for renderable %u, doesn't exist!\n", id);
    return;
  }

  it->second._transform = transform;
}

}