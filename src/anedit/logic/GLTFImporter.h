#pragma once

#include <render/asset/Prefab.h>
#include <render/asset/Model.h>
#include <render/asset/Texture.h>
#include <render/asset/Material.h>
#include <render/animation/Animation.h>

#include <memory>
#include <future>
#include <filesystem>
#include <vector>

namespace logic {

struct LoadedGLTFData
{
  // These prefabs will be pre-filled with the correct model, materials and skeleton id.
  std::vector<render::asset::Prefab> _prefabs;

  std::vector<render::asset::Model> _models;
  std::vector<render::asset::Texture> _textures;
  std::vector<render::asset::Material> _materials;

  std::vector<render::anim::Animation> _animations;
};

// Asynchronously loads GLTF data from a path.
class GLTFImporter
{
public:
  GLTFImporter();
  ~GLTFImporter();

  GLTFImporter(const GLTFImporter&) = delete;
  GLTFImporter(GLTFImporter&&) = delete;
  GLTFImporter& operator=(const GLTFImporter&) = delete;
  GLTFImporter& operator=(GLTFImporter&&) = delete;

  void startLoad(const std::filesystem::path& path);
  bool hasResult() const;

  // Gives nullptr if there is no result
  std::unique_ptr<LoadedGLTFData> takeResult();
  
private:
  std::future<LoadedGLTFData> _loadingFuture;
};

}