#pragma once

#include <entt/entt.hpp>

namespace util {
  class Uuid;
}

namespace render::scene {
  class Scene;
}

namespace render::asset {
  class AssetCollection;
}

namespace terrain {

class TerrainSystem
{
public:
  TerrainSystem(render::scene::Scene* scene, render::asset::AssetCollection* assColl);
  TerrainSystem() = default;
  ~TerrainSystem() = default;

  TerrainSystem(const TerrainSystem&) = delete;
  TerrainSystem(TerrainSystem&&) = delete;
  TerrainSystem& operator=(const TerrainSystem&) = delete;
  TerrainSystem& operator=(TerrainSystem&&) = delete;

  void setScene(render::scene::Scene* scene);
  void setAssetCollection(render::asset::AssetCollection* assColl);

  void update();

private:
  void generateModel(util::Uuid& node);

  render::scene::Scene* _scene = nullptr;
  render::asset::AssetCollection* _assColl = nullptr;
  entt::observer _observer;
};

}