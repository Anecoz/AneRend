#pragma once

#include "../Identifiers.h"
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

  void addRenderable(RenderableId id);
  void removeRenderable(RenderableId id);
  const std::vector<RenderableId>& getRenderableIds() const { return _renderables; }

  static const unsigned _tileSize = 32; // in world space
  static TileIndex posToIdx(const glm::vec3& pos);

private:
  TileIndex _index;
  bool _initialized = false;

  // TODO: Lights, particles
  std::vector<RenderableId> _renderables;

};

}