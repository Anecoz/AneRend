#pragma once

#include "../../util/Uuid.h"
#include "TileIndex.h"

#include <glm/glm.hpp>

#include <vector>

namespace render { class RenderContext; }

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

  void update(const glm::vec3& pos);

private:
  render::RenderContext* _rc = nullptr;
  Scene* _scene = nullptr;

  // How many tiles around current camera tile do we page:
  int _pageRadius = 2;
  std::vector<TileIndex> _pagedTiles;

  std::vector<util::Uuid> _pagedModels;
  std::vector<util::Uuid> _pagedMaterials;
  std::vector<util::Uuid> _pagedAnimations;
  std::vector<util::Uuid> _pagedSkeletons;
};

}