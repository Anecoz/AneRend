#include "Tile.h"

namespace render::scene {

Tile::Tile()
  : _index(0, 0)
  , _initialized(false)
{}

Tile::Tile(const TileIndex& index)
  : _index(index)
  , _initialized(true)
{}

Tile::~Tile()
{}

Tile::Tile(Tile&& rhs)
{
  std::swap(_nodes, rhs._nodes);
  std::swap(_dirty, rhs._dirty);
  std::swap(_dirtyNodes, rhs._dirtyNodes);
  std::swap(_removedNodes, rhs._removedNodes);
  std::swap(_ddgiAtlas, rhs._ddgiAtlas);
  std::swap(_index, rhs._index);
  _initialized = true;
  rhs._initialized = false;
}

Tile& Tile::operator=(Tile&& rhs)
{
  if (this != &rhs) {
    std::swap(_nodes, rhs._nodes);
    std::swap(_dirty, rhs._dirty);
    std::swap(_dirtyNodes, rhs._dirtyNodes);
    std::swap(_removedNodes, rhs._removedNodes);
    std::swap(_ddgiAtlas, rhs._ddgiAtlas);
    std::swap(_index, rhs._index);
    _initialized = true;
    rhs._initialized = false;
  }

  return *this;
}

Tile::operator bool() const
{
  return _initialized;
}

void Tile::addNode(util::Uuid id)
{
  _nodes.emplace_back(id);
  _dirtyNodes.emplace_back(id);
}

void Tile::removeNode(util::Uuid id)
{
  _nodes.erase(std::remove(_nodes.begin(), _nodes.end(), id), _nodes.end());
  _removedNodes.emplace_back(std::move(id));
  _dirty = true;
}

TileIndex Tile::posToIdx(const glm::vec3& pos)
{
  // Project translation to y=0 plane
  int tileX = (int)(pos.x / (float)_tileSize);
  int tileY = (int)(pos.z / (float)_tileSize);

  return { tileX, tileY };
}

}