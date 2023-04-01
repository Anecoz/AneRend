#pragma once

#include <glm/glm.hpp>

#include "../util/noiseutils.h"
#include <noise/noise.h>

namespace logic {

struct WindMap
{
  float* data;
  int stride;
  int width;
  int height;

  glm::vec2 windDir;
};

class WindSystem
{
public:
  WindSystem();
  ~WindSystem();

  void setWindDir(const glm::vec2& windDir);
  void update(double delta);

  WindMap getCurrentWindMap();

private:
  glm::vec2 _windDir;

  module::Perlin _perlin;
  utils::NoiseMap _heightMap;
  utils::NoiseMapBuilderPlane _heightMapBuilder;
};

}