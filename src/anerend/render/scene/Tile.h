#pragma once

#include "../../util/Uuid.h"
#include "TileIndex.h"

#include <glm/glm.hpp>

#include <vector>

namespace render::scene {

class Tile
{
public:
  Tile();
  Tile(const TileIndex& index);
  ~Tile();

  Tile(Tile&&);
  Tile& operator=(Tile&&);

  // Copying is not allowed.
  Tile(const Tile&) = delete;
  Tile& operator=(const Tile&) = delete;

  explicit operator bool() const;

  void addRenderable(util::Uuid id);
  void removeRenderable(util::Uuid id);
  const std::vector<util::Uuid>& getRenderableIds() const { return _renderables; }

  static const unsigned _tileSize = 32; // in world space
  static TileIndex posToIdx(const glm::vec3& pos);

private:
  TileIndex _index;
  bool _initialized = false;

  // TODO: Lights, particles
  std::vector<util::Uuid> _renderables;

};

}