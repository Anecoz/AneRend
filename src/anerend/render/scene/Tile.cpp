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
  std::swap(_renderables, rhs._renderables);
  std::swap(_lights, rhs._lights);
  std::swap(_ddgiAtlas, rhs._ddgiAtlas);
  std::swap(_index, rhs._index);
  _initialized = true;
  rhs._initialized = false;
}

Tile& Tile::operator=(Tile&& rhs)
{
  if (this != &rhs) {
    std::swap(_renderables, rhs._renderables);
    std::swap(_lights, rhs._lights);
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

void Tile::addRenderable(util::Uuid id)
{
  _renderables.emplace_back(id);
}

void Tile::removeRenderable(util::Uuid id)
{
  _renderables.erase(std::remove(_renderables.begin(), _renderables.end(), id), _renderables.end());
}

void Tile::addLight(util::Uuid id)
{
  _lights.emplace_back(id);
}

void Tile::removeLight(util::Uuid id)
{
  _lights.erase(std::remove(_lights.begin(), _lights.end(), id), _lights.end());
}

TileIndex Tile::posToIdx(const glm::vec3& pos)
{
  // Project translation to y=0 plane
  int tileX = (int)(pos.x / (float)_tileSize);
  int tileY = (int)(pos.z / (float)_tileSize);

  return { tileX, tileY };
}

}