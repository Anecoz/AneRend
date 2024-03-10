#pragma once

#include "../../util/Uuid.h"

#include <mutex>
#include <unordered_map>
#include <vector>

namespace render::asset {

class AssetCollection;

struct Texture;
struct Material;
struct Model;

class AssetFetcher
{
public:
  AssetFetcher() = default;
  AssetFetcher(AssetCollection* assColl);
  ~AssetFetcher() = default;

  AssetFetcher(const AssetFetcher&) = delete;
  AssetFetcher(AssetFetcher&&) = delete;
  AssetFetcher& operator=(const AssetFetcher&) = delete;
  AssetFetcher& operator=(AssetFetcher&&) = delete;

  void setAssetCollection(AssetCollection* assColl) { _assColl = assColl; }

  int ref(const util::Uuid& id);
  int checkRef(const util::Uuid& id);
  int deref(const util::Uuid& id);

  // If already refed, these will not start fetching.
  void startFetchTexture(const util::Uuid& id, bool doRef = true);
  void startFetchMaterial(const util::Uuid& id, bool autoRefTextures = true);
  void startFetchModel(const util::Uuid& id);

  std::vector<Texture> takeTextures();
  std::vector<Model> takeModels();
  std::vector<Material> takeMaterials();

private:
  AssetCollection* _assColl = nullptr;

  std::mutex _mtx;
  std::mutex _refMtx;

  std::unordered_map<util::Uuid, int> _ref;

  std::vector<Texture> _fetchedTextures;
  std::vector<Model> _fetchedModels;
  std::vector<Material> _fetchedMaterials;
};

}