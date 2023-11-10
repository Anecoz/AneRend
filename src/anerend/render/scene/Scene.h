#pragma once

#include "Tile.h"
#include "TileIndex.h"
#include "DeserialisedSceneData.h"

#include "../Identifiers.h"
#include "../asset/Model.h"
#include "../asset/Renderable.h"
#include "../asset/Material.h"
#include "../asset/Animator.h"

#include "../animation/Skeleton.h"
#include "../animation/Animation.h"

#include "internal/SceneSerializer.h"

#include <filesystem>
#include <future>
#include <memory>
#include <vector>

namespace render::scene {

enum class SceneEventType
{
  ModelAdded,
  ModelRemoved,
  AnimationAdded,
  AnimationRemoved,
  AnimatorAdded,
  AnimatorUpdated,
  AnimatorRemoved,
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

  // This will take a snapshot of the scene as it is at call time,
  // and then asynchronously serialize to path.
  void serializeAsync(const std::filesystem::path& path);

  std::future<DeserialisedSceneData> deserializeAsync(const std::filesystem::path& path);

  const SceneEventLog& getEvents() const;
  void resetEvents();

  bool getTile(TileIndex idx, Tile** tileOut);

  const std::vector<asset::Material>& getMaterials() const {
    return _materials;
  }

  const std::vector<asset::Model>& getModels() const {
    return _models;
  }

  const std::vector<anim::Animation>& getAnimations() const {
    return _animations;
  }

  const std::vector<anim::Skeleton>& getSkeletons() const {
    return _skeletons;
  }

  const std::vector<asset::Animator>& getAnimators() const {
    return _animators;
  }

  const std::vector<asset::Renderable>& getRenderables() const {
    return _renderables;
  }

  ModelId addModel(asset::Model&& model, bool genId = true);
  void removeModel(ModelId id);
  const asset::Model* getModel(ModelId id);

  MaterialId addMaterial(asset::Material&& material, bool genId = true);
  void removeMaterial(MaterialId id);
  const asset::Material* getMaterial(MaterialId id);

  AnimationId addAnimation(anim::Animation&& animation, bool genId = true);
  void removeAnimation(AnimationId id);
  const anim::Animation* getAnimation(AnimationId id);

  SkeletonId addSkeleton(anim::Skeleton&& skeleton, bool genId = true);
  void removeSkeleton(SkeletonId id);
  const anim::Skeleton* getSkeleton(SkeletonId id);

  AnimatorId addAnimator(asset::Animator&& animator, bool genId = true);
  void updateAnimator(asset::Animator animator);
  void removeAnimator(AnimatorId id);
  const asset::Animator* getAnimator(AnimatorId id);

  RenderableId addRenderable(asset::Renderable&& renderable, bool genId = true);
  void removeRenderable(RenderableId id);
  const asset::Renderable* getRenderable(RenderableId id);

  // TODO: Decide if the Scene class really is the correct place to access/modify renderables.
  void setRenderableTint(RenderableId id, const glm::vec3& tint);
  void setRenderableTransform(RenderableId id, const glm::mat4& transform);

private:
  friend struct internal::SceneSerializer;

  std::unordered_map<TileIndex, Tile> _tiles;

  /*
  * TODO: In the future maybe these need to be tile - based aswell.
  * They may need to be streamed in on demand, but that should be decided using renderables
  * _using_ the assets.
  * Renderables aren't really assets... Maybe assets don't belong here, only renderables, lights, particle emitters etc.
  */ 
  std::vector<asset::Model> _models;
  std::vector<asset::Material> _materials;
  std::vector<anim::Animation> _animations;
  std::vector<anim::Skeleton> _skeletons;
  std::vector<asset::Animator> _animators;
  std::vector<asset::Renderable> _renderables;

  SceneEventLog _eventLog;
  internal::SceneSerializer _serialiser;

  void addEvent(SceneEventType type, uint32_t id, TileIndex tileIdx = TileIndex());
};

}