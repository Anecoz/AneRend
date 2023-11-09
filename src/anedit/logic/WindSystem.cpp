#include "WindSystem.h"

namespace logic {

WindSystem::WindSystem()
  : _windDir(1.0f, 0.0f)
{
  _perlin.SetOctaveCount(3);

  _heightMapBuilder.SetSourceModule(_perlin);
  _heightMapBuilder.SetDestNoiseMap(_heightMap);
  _heightMapBuilder.SetDestSize(256, 256);
  _heightMapBuilder.SetBounds(2.0, 6.0, 1.0, 5.0);
  _heightMapBuilder.Build();
}

WindSystem::~WindSystem()
{}

void WindSystem::setWindDir(const glm::vec2& windDir)
{
  _windDir = windDir;
}

void WindSystem::update(double delta)
{
  const double speed = 0.5;

  // Scroll in wind direction and rebuild
  double prevLowerX = _heightMapBuilder.GetLowerXBound();
  double prevUpperX = _heightMapBuilder.GetUpperXBound();
  double prevLowerZ = _heightMapBuilder.GetLowerZBound();
  double prevUpperZ = _heightMapBuilder.GetUpperZBound();
  _heightMapBuilder.SetBounds(
    prevLowerX + _windDir.x * delta * speed,
    prevUpperX + _windDir.x * delta * speed,
    prevLowerZ + _windDir.y * delta * speed,
    prevUpperZ + _windDir.y * delta * speed);

  _heightMapBuilder.Build();
}

WindMap WindSystem::getCurrentWindMap()
{
  WindMap out{};
  out.data = _heightMap.GetSlabPtr();
  out.height = _heightMap.GetHeight();
  out.width = _heightMap.GetWidth();
  out.stride = _heightMap.GetStride();
  out.windDir = _windDir;
  return out;
}

}