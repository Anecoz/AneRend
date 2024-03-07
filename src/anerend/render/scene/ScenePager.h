#pragma once

#include "../../util/Uuid.h"
#include "TileIndex.h"
#include "../asset/Model.h"
#include "../asset/Texture.h"
#include "../asset/Material.h"
#include "../../component/Components.h"

#include <glm/glm.hpp>

#include <mutex>
#include <vector>

namespace render { class RenderContext; }
namespace render::asset { class AssetCollection; }

namespace render::scene {

class Scene;

class ScenePager
{
public:
  ScenePager();
  ScenePager(render::RenderContext* rc);
  ~ScenePager();

  // No copy or move
  ScenePager(const ScenePager&) = delete;
  ScenePager(ScenePager&&) = delete;
  ScenePager& operator=(const ScenePager&) = delete;
  ScenePager& operator=(ScenePager&&) = delete;

  void setScene(Scene* scene);
  void setAssetCollection(asset::AssetCollection* assColl);

  void update(const glm::vec3& pos);

private:
  struct PendingRenderableAssets
  {
    util::Uuid _node;

    bool _modelSet = false;
    int _requestedTextures = -1;
    int _requestedMaterials = -1;

    asset::Model _model;
    std::vector<asset::Texture> _textures;
    std::vector<asset::Material> _materials;

    bool isComplete() const {
      if (!_modelSet) return false;
      if (_requestedTextures == -1) return false;
      if (_requestedMaterials == -1) return false;

      return
        _requestedTextures == (int)_textures.size() &&
        _requestedMaterials == (int)_materials.size();
    }
  };

  void page(const util::Uuid& node);
  void unpage(const util::Uuid& node);

  render::RenderContext* _rc = nullptr;
  Scene* _scene = nullptr;
  asset::AssetCollection* _assColl = nullptr;

  // How many tiles around current camera tile do we page:
  int _pageRadius = 10;
  std::vector<TileIndex> _pagedTiles;
};

}