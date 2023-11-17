#pragma once

#include "Tile.h"
#include "TileIndex.h"
#include "DeserialisedSceneData.h"

#include "../../util/Uuid.h"
#include "../asset/Model.h"
#include "../asset/Renderable.h"
#include "../asset/Material.h"
#include "../asset/Animator.h"
#include "../asset/Prefab.h"

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
  PrefabAdded,
  PrefabRemoved,
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
  util::Uuid _id;
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

  const std::vector<asset::Prefab>& getPrefabs() const {
    return _prefabs;
  }

  const std::vector<asset::Renderable>& getRenderables() const {
    return _renderables;
  }

  util::Uuid addPrefab(asset::Prefab&& prefab);
  void removePrefab(util::Uuid id);
  const asset::Prefab* getPrefab(util::Uuid id);

  util::Uuid addModel(asset::Model&& model);
  void removeModel(util::Uuid id);
  const asset::Model* getModel(util::Uuid id);

  util::Uuid addMaterial(asset::Material&& material);
  void removeMaterial(util::Uuid id);
  const asset::Material* getMaterial(util::Uuid id);

  util::Uuid addAnimation(anim::Animation&& animation);
  void removeAnimation(util::Uuid id);
  const anim::Animation* getAnimation(util::Uuid id);

  util::Uuid addSkeleton(anim::Skeleton&& skeleton);
  void removeSkeleton(util::Uuid id);
  const anim::Skeleton* getSkeleton(util::Uuid id);

  util::Uuid addAnimator(asset::Animator&& animator);
  void updateAnimator(asset::Animator animator);
  void removeAnimator(util::Uuid id);
  const asset::Animator* getAnimator(util::Uuid id);

  util::Uuid addRenderable(asset::Renderable&& renderable);
  void removeRenderable(util::Uuid id);
  const asset::Renderable* getRenderable(util::Uuid id);

  // TODO: Decide if the Scene class really is the correct place to access/modify renderables.
  void setRenderableTint(util::Uuid id, const glm::vec3& tint);
  void setRenderableTransform(util::Uuid id, const glm::mat4& transform);

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
  std::vector<asset::Prefab> _prefabs;
  std::vector<asset::Renderable> _renderables;

  SceneEventLog _eventLog;
  internal::SceneSerializer _serialiser;

  void addEvent(SceneEventType type, util::Uuid id, TileIndex tileIdx = TileIndex());
};

}