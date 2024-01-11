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

  bool& dirty() { return _dirty; }
  std::vector<util::Uuid>& getDirtyNodes() { return _dirtyNodes; }
  std::vector<util::Uuid>& getRemovedNodes() { return _removedNodes; }

  void addNode(util::Uuid id);
  void removeNode(util::Uuid id);
  std::vector<util::Uuid>& getNodes() { return _nodes; }

  void setDDGIAtlas(util::Uuid id) { _ddgiAtlas = id; }
  util::Uuid getDDGIAtlas() { return _ddgiAtlas; }

  static const unsigned _tileSize = 32; // in world space
  static TileIndex posToIdx(const glm::vec3& pos);

private:
  bool _dirty = true;
  TileIndex _index;
  bool _initialized = false;

  std::vector<util::Uuid> _nodes;
  std::vector<util::Uuid> _dirtyNodes; // These are also located in _nodes
  std::vector<util::Uuid> _removedNodes;
  util::Uuid _ddgiAtlas;

};

}