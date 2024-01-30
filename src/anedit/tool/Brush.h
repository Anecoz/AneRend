#pragma once

#include <cstdint>

namespace tool {

struct Brush
{
  // In pixels, falloff included.
  unsigned _radius = 20;

  // This is the max opacity of the brush.
  std::uint8_t _baseOpacity = 255;

  // Num pixels where brush fades from _baseOpacity to 0.
  unsigned _falloff = 5;
};

}