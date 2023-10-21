#pragma once

#include "Tile.h"

#include "../Identifiers.h"
#include "../asset/Model.h"
#include "../asset/Renderable.h"

#include "../animation/Animation.h"

#include <vector>
#include <unordered_map>

namespace render::scene {

class Scene
{
public:
  Scene();
  ~Scene();

  Scene(Scene&&);
  Scene& operator=(Scene&&);

  // Copying is not allowed.
  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;

  ModelId addModel(asset::Model&& model);
  void removeModel(ModelId id);

  AnimationId addAnimation(anim::Animation&& animation);
  void removeAnimation(AnimationId id);

  RenderableId addRenderable(asset::Renderable&& renderable);
  void removeRenderable(RenderableId id);
  void setRenderableTint(RenderableId id, const glm::vec3& tint);
  void setRenderableTransform(RenderableId id, const glm::mat4& transform);

private:
  const unsigned _tileSize = 32; // in world space
  std::unordered_map<glm::ivec2, Tile> _tiles;

  // TODO: In the future maybe these need to be tile-based aswell.
  std::unordered_map<ModelId, asset::Model> _models;
  //std::unordered_map<MeshId, Mesh> _meshes;
  std::unordered_map<AnimationId, anim::Animation> _animations;
  std::unordered_map<RenderableId, asset::Renderable> _renderables;

  /*ModelId _currentModelId = INVALID_ID;
  MeshId _currentMeshid = INVALID_ID;
  AnimationId _currentAnimId = INVALID_ID;
  RenderableId _currentRenderableId = INVALID_ID;*/
};

}