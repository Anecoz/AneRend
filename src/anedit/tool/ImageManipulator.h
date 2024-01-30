#pragma once

#include "Brush.h"

#include <render/asset/Texture.h>

#include <glm/glm.hpp>

#include <functional>

namespace tool {

class ImageManipulator
{
public:
  ImageManipulator() = delete;
  ~ImageManipulator() = default;
  ImageManipulator(Brush brush);

  ImageManipulator(const ImageManipulator&) = delete;
  ImageManipulator(ImageManipulator&&) = delete;
  ImageManipulator& operator=(const ImageManipulator&) = delete;
  ImageManipulator& operator=(ImageManipulator&&) = delete;

  struct PaintMask
  {
    bool _r = true;
    bool _g = true;
    bool _b = true;
    bool _a = true;
  };

  // Will only paint in mip 0!
  void paint8Bit(
    render::asset::Texture& texture,
    unsigned x, unsigned y,
    const glm::u8vec4& value,
    PaintMask mask = PaintMask(),
    bool keepMax = true);

  // Will only paint in mip 0!
  void paint8Bit(
    render::asset::Texture& texture,
    unsigned x, unsigned y,
    std::function<void(glm::u8vec4*, float)> cb); // old val and falloff

private:
  Brush _brush;
};

}