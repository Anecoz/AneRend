#include "Tile.h"

namespace render::scene {

Tile::Tile()
  : _index({0, 0})
{
}

Tile::Tile(const glm::ivec2& index)
  : _index(index)
  , _initialized(true)
{}

Tile::~Tile()
{}

Tile::Tile(Tile&& rhs)
{
  std::swap(_renderables, rhs._renderables);
  std::swap(_index, rhs._index);
  _initialized = true;
  rhs._initialized = false;
}

Tile& Tile::operator=(Tile&& rhs)
{
  if (this != &rhs) {
    std::swap(_renderables, rhs._renderables);
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

void Tile::addRenderable(RenderableId id)
{
  _renderables.emplace_back(id);
}

void Tile::removeRenderable(RenderableId id)
{
  _renderables.erase(std::remove(_renderables.begin(), _renderables.end(), id), _renderables.end());
}

}