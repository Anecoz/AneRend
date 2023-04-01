#pragma once

#include <string>

namespace render {

struct RenderDebugOptions
{
  bool debugView = true;
  float windStrength = 1.0f;
  std::string debugViewResource;
};

}