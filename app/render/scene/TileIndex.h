#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace render::scene {

struct TileIndex
{
  TileIndex() : _idx(0, 0) {}
  TileIndex(int x, int z) : _idx(x, z), _initialized(true) {}

  explicit operator bool() const {
    return _initialized;
  }

  bool operator==(const TileIndex& rhs) const {
    return _idx == rhs._idx;
  }

  glm::ivec2 _idx;
  bool _initialized = false;
};

}

namespace std {
  template <>
  struct hash<render::scene::TileIndex> {
    size_t operator()(const render::scene::TileIndex& tileIndex) const {
      return hash<glm::ivec2>()(tileIndex._idx);
    }
  };
}