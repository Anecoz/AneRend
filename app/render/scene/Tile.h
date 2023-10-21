#pragma once

#include "../Identifiers.h"

#include <glm/glm.hpp>

#include <vector>

namespace render::scene {

class Tile
{
public:
  Tile();
  Tile(const glm::ivec2& index);
  ~Tile();

  Tile(Tile&&);
  Tile& operator=(Tile&&);

  // Copying is not allowed.
  Tile(const Tile&) = delete;
  Tile& operator=(const Tile&) = delete;

  explicit operator bool() const;

  void addRenderable(RenderableId id);
  void removeRenderable(RenderableId id);

private:
  glm::ivec2 _index;
  bool _initialized = false;

  // TODO: Lights, particles
  std::vector<RenderableId> _renderables;

};

}