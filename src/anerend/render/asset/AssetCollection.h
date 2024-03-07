#pragma once

#include "Model.h"
#include "Material.h"
#include "Prefab.h"
#include "Texture.h"
#include "Cinematic.h"
#include "../animation/Animation.h"

#include <filesystem>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace render::asset {

typedef std::function<void(Model)> ModelRetrievedCallback;
typedef std::function<void(Material)> MaterialRetrievedCallback;
typedef std::function<void(Prefab)> PrefabRetrievedCallback;
typedef std::function<void(Texture)> TextureRetrievedCallback;
typedef std::function<void(Cinematic)> CinematicRetrievedCallback;
typedef std::function<void(anim::Animation)> AnimationRetrievedCallback;

// Meta info for assets stored on disk.
struct AssetMetaInfo
{
  enum Type : std::uint8_t
  {
    Material,
    Model,
    Animation,
    Prefab,
    Texture,
    Cinematic
  } _type;

  util::Uuid _id;
  std::string _name;
  std::size_t _offset;
  std::size_t _sizeOnDisk;
};

enum class AssetEventType
{
  MaterialUpdated,
  PrefabUpdated,
  TextureUpdated,
  CinematicUpdated
};

struct AssetEvent
{
  AssetEventType _type;
  util::Uuid _id;
};

struct AssetEventLog
{
  std::vector<AssetEvent> _events;
};

class AssetCollection
{
public:
  AssetCollection();
  AssetCollection(std::filesystem::path p);
  ~AssetCollection();

  AssetCollection(const AssetCollection&) = delete;
  AssetCollection& operator=(const AssetCollection&) = delete;
  AssetCollection(AssetCollection&&);
  AssetCollection& operator=(AssetCollection&&);

  const AssetEventLog& getEventLog() const;
  void clearEventLog();

  // Builds an index from the file located at provided path.
  void readIndices();

  explicit operator bool() const;

  // Meta info getters.
  const std::vector<AssetMetaInfo>& getMetaInfos(AssetMetaInfo::Type type);

  // These will add to cache.
  void add(Model a);
  void add(Material a);
  void add(Prefab a);
  void add(Texture a);
  void add(Cinematic a);
  void add(anim::Animation a);

  // Will update only what is in cache.
  // TODO: Some of these need to be picked up by things like the renderer.
  void updateMaterial(Material a);
  void updatePrefab(Prefab a);
  void updateTexture(Texture a);
  void updateCinematic(Cinematic a);

  // Will only remove what is in cache.
  void removeModel(const util::Uuid& id);
  void removeMaterial(const util::Uuid& id);
  void removePrefab(const util::Uuid& id);
  void removeTexture(const util::Uuid& id);
  void removeCinematic(const util::Uuid& id);
  void removeAnimation(const util::Uuid& id);

  // These will get either from cache or stream from disk.
  // The callback may be called from a worker thread.
  void getModel(const util::Uuid& id, ModelRetrievedCallback cb);
  void getMaterial(const util::Uuid& id, MaterialRetrievedCallback cb);
  void getPrefab(const util::Uuid& id, PrefabRetrievedCallback cb);
  void getTexture(const util::Uuid& id, TextureRetrievedCallback cb);
  void getCinematic(const util::Uuid& id, CinematicRetrievedCallback cb);
  void getAnimation(const util::Uuid& id, AnimationRetrievedCallback cb);

  // These will also get from cache or disk, but will block.
  Model getModelBlocking(const util::Uuid& id);
  Material getMaterialBlocking(const util::Uuid& id);
  Prefab getPrefabBlocking(const util::Uuid& id);
  Texture getTextureBlocking(const util::Uuid& id);
  Cinematic getCinematicBlocking(const util::Uuid& id);
  anim::Animation getAnimationBlocking(const util::Uuid& id);

  // Will take whatever is cached (+ on disk) and serialise it down to the provided path.
  // Typically used when creating the AssetCollection, not during gameplay.
  void serialiseToPath(std::filesystem::path p = {});

  // Prints to stdout.
  void printDebugInfo();

private:
  AssetEventLog _log;
  void addEvent(AssetEventType type, const util::Uuid& id);

  struct FileIndex
  {
    std::unordered_map<util::Uuid, AssetMetaInfo> _map;
  } _fileIndex;

  // Holds meta info for each type, for convenience.
  std::unordered_map<AssetMetaInfo::Type, std::vector<AssetMetaInfo>> _metaInfos;

  template <typename T>
  bool readIndexAsync(const util::Uuid& id, std::function<void(T)> cb, std::vector<T>& cache);

  template <typename T>
  T readIndexBlocking(const util::Uuid& id, std::vector<T>& cache);

  template <typename T>
  void getAssetAsync(const util::Uuid& id, std::function<void(T)> cb, std::vector<T>& cache);

  template <typename T>
  T getAssetBlocking(const util::Uuid& id, std::vector<T>& cache);

  template <typename T>
  void addAsset(T asset, std::vector<T>& cache, AssetMetaInfo::Type type);

  template <typename T>
  void removeAsset(const util::Uuid& id, std::vector<T>& cache);

  template <typename T>
  void updateAsset(T asset, std::vector<T>& cache, AssetMetaInfo::Type type);

  template <typename T>
  std::vector<util::Uuid> buildCacheId(const std::vector<T>& cache) const;

  std::filesystem::path _p;

  // Needs to be held whenever accessing non-const functions of _cachePtr or the cache vectors.
  mutable std::mutex _cacheMtx;

  // Contains index into the vectors below, if the asset is cached.
  std::unordered_map<util::Uuid, std::size_t> _cachePtr;

  // Size on disk of the serialised indices, used to find where to start looking for
  // the actual assets.
  std::uint32_t _indicesFileSize = 0;

  // Holds the actual cached assets.
  std::vector<Model> _cachedModels;
  std::vector<Material> _cachedMaterials;
  std::vector<Prefab> _cachedPrefabs;
  std::vector<Texture> _cachedTextures;
  std::vector<Cinematic> _cachedCinematics;
  std::vector<anim::Animation> _cachedAnimations;
};

}