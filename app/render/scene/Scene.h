#pragma once

#include "Tile.h"
#include "TileIndex.h"

#include "../Identifiers.h"
#include "../asset/Model.h"
#include "../asset/Renderable.h"
#include "../asset/Material.h"

#include "../animation/Skeleton.h"
#include "../animation/Animation.h"

#include <vector>

namespace render::scene {

enum class SceneEventType
{
  ModelAdded,
  ModelRemoved,
  AnimationAdded,
  AnimationRemoved,
  SkeletonAdded,
  SkeletonRemoved,
  MaterialAdded,
  MaterialRemoved,
  RenderableAdded,
  RenderableUpdated,
  RenderableRemoved
};

struct SceneEvent
{
  SceneEventType _type;
  TileIndex _tileIdx;
  uint32_t _id; // cast this to appropriate id
};

struct SceneEventLog
{
  std::vector<SceneEvent> _events;
};

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

  const SceneEventLog& getEvents() const;
  void resetEvents();

  bool getTile(TileIndex idx, Tile** tileOut);

  ModelId addModel(asset::Model&& model);
  void removeModel(ModelId id);
  const asset::Model* getModel(ModelId id);

  MaterialId addMaterial(asset::Material&& material);
  void removeMaterial(MaterialId id);
  const asset::Material* getMaterial(MaterialId id);

  AnimationId addAnimation(anim::Animation&& animation);
  void removeAnimation(AnimationId id);
  const anim::Animation* getAnimation(AnimationId id);

  SkeletonId addSkeleton(anim::Skeleton&& skeleton);
  void removeSkeleton(SkeletonId id);
  const anim::Skeleton* getSkeleton(SkeletonId id);

  RenderableId addRenderable(asset::Renderable&& renderable);
  void removeRenderable(RenderableId id);
  const asset::Renderable* getRenderable(RenderableId id);

  // TODO: Decide if the Scene class really is the correct place to access/modify renderables.
  void setRenderableTint(RenderableId id, const glm::vec3& tint);
  void setRenderableTransform(RenderableId id, const glm::mat4& transform);

private:
  std::unordered_map<TileIndex, Tile> _tiles;

  // TODO: In the future maybe these need to be tile-based aswell.
  std::vector<asset::Model> _models;
  std::vector<asset::Material> _materials;
  std::vector<anim::Animation> _animations;
  std::vector<anim::Skeleton> _skeletons;
  std::vector<asset::Renderable> _renderables;

  SceneEventLog _eventLog;

  void addEvent(SceneEventType type, uint32_t id, TileIndex tileIdx = TileIndex());
};

}