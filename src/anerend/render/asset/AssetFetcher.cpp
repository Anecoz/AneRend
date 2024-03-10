#include "AssetFetcher.h"

#include "AssetCollection.h"

#include "Texture.h"
#include "Material.h"
#include "Model.h"

namespace render::asset {

AssetFetcher::AssetFetcher(AssetCollection* assColl)
  : _assColl(assColl)
{}

int AssetFetcher::ref(const util::Uuid& id)
{
  std::lock_guard<std::mutex> lock(_refMtx);
  return ++_ref[id];
}

int AssetFetcher::checkRef(const util::Uuid& id)
{
  std::lock_guard<std::mutex> lock(_refMtx);
  int ref = -1;
  if (_ref.contains(id)) {
    ref = _ref[id];
  }

  return ref;
}

int AssetFetcher::deref(const util::Uuid& id)
{
  std::lock_guard<std::mutex> lock(_refMtx);
  int ref = -1;
  if (_ref.contains(id)) {
    ref = --_ref[id];

    if (ref == 0) {
      _ref.erase(id);
    }
  }

  return ref;
}

void AssetFetcher::startFetchTexture(const util::Uuid& id, bool doRef)
{
  if (checkRef(id) > 0) {
    if (doRef) {
      ref(id);
    }
    return;
  }

  if (doRef) {
    ref(id);
  }

  _assColl->getTexture(id, [this](Texture tex) {
    std::lock_guard<std::mutex> lock(_mtx);
    _fetchedTextures.emplace_back(std::move(tex));
  });
}

void AssetFetcher::startFetchMaterial(const util::Uuid& id, bool autoRefTextures)
{
  if (checkRef(id) > 0) return;

  _assColl->getMaterial(id, [this, autoRef = autoRefTextures](Material mat) {
    // Start loading the associated textures.
    if (mat._albedoTex) {
      startFetchTexture(mat._albedoTex, autoRef);
    }
    if (mat._metallicRoughnessTex) {
      startFetchTexture(mat._metallicRoughnessTex, autoRef);
    }
    if (mat._normalTex) {
      startFetchTexture(mat._normalTex, autoRef);
    }
    if (mat._emissiveTex) {
      startFetchTexture(mat._emissiveTex, autoRef);
    }

    std::lock_guard<std::mutex> lock(_mtx);
    _fetchedMaterials.emplace_back(std::move(mat));
  });
}

void AssetFetcher::startFetchModel(const util::Uuid& id)
{
  if (checkRef(id) > 0) return;

  _assColl->getModel(id, [this](Model model) {
    std::lock_guard<std::mutex> lock(_mtx);
    _fetchedModels.emplace_back(std::move(model));
  });
}

std::vector<Texture> AssetFetcher::takeTextures()
{
  std::lock_guard<std::mutex> lock(_mtx);
  return std::move(_fetchedTextures);
}

std::vector<Model> AssetFetcher::takeModels()
{
  std::lock_guard<std::mutex> lock(_mtx);
  return std::move(_fetchedModels);
}

std::vector<Material> AssetFetcher::takeMaterials()
{
  std::lock_guard<std::mutex> lock(_mtx);
  return std::move(_fetchedMaterials);
}

}