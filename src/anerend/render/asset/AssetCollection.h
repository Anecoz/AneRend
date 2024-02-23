#pragma once

#include "Model.h"
#include "Material.h"
#include "Prefab.h"
#include "Texture.h"
#include "Cinematic.h"

#include <filesystem>
#include <functional>
#include <unordered_map>
#include <vector>

namespace render::asset {

typedef std::function<void(Model)> ModelRetrievedCallback;
typedef std::function<void(Material)> MaterialRetrievedCallback;
typedef std::function<void(Prefab)> PrefabRetrievedCallback;
typedef std::function<void(Texture)> TextureRetrievedCallback;
typedef std::function<void(Cinematic)> CinematicRetrievedCallback;

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

  explicit operator bool() const;

  // These will add to cache.
  void add(Model a);
  void add(Material a);
  void add(Prefab a);
  void add(Texture a);
  void add(Cinematic a);

  // Will update only what is in cache.
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

  // These will get either from cache or stream from disk.
  // The callback may be called from a worker thread.
  void getModel(const util::Uuid& id, ModelRetrievedCallback cb);
  void getMaterial(const util::Uuid& id, MaterialRetrievedCallback cb);
  void getPrefab(const util::Uuid& id, PrefabRetrievedCallback cb);
  void getTexture(const util::Uuid& id, TextureRetrievedCallback cb);
  void getCinematic(const util::Uuid& id, CinematicRetrievedCallback cb);

  // Will take whatever is cached and serialise it down to the provided path.
  // Typically used when creating the AssetCollection, not during gameplay.
  void serialiseCacheToPath();

  // Prints to stdout.
  void printDebugInfo();

private:
  struct FileIndex
  {
    // The size_t:s are offsets into the file on disk.
    std::unordered_map<util::Uuid, std::size_t> _map;
  } _fileIndex;

  // Builds an index from the file located at _p.
  void readIndices();

  std::filesystem::path _p;

  // Contains index into the vectors below, if the asset is cached.
  std::unordered_map<util::Uuid, std::size_t> _modelCachePtr;
  std::unordered_map<util::Uuid, std::size_t> _materialCachePtr;
  std::unordered_map<util::Uuid, std::size_t> _prefabCachePtr;
  std::unordered_map<util::Uuid, std::size_t> _textureCachePtr;
  std::unordered_map<util::Uuid, std::size_t> _cinematicCachePtr;

  // Holds the actual cached assets.
  std::vector<Model> _cachedModels;
  std::vector<Material> _cachedMaterials;
  std::vector<Prefab> _cachedPrefabs;
  std::vector<Texture> _cachedTextures;
  std::vector<Cinematic> _cachedCinematics;
};

}