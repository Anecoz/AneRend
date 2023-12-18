#pragma once

#include "../../util/Uuid.h"

#include <cstdint>
#include <vector>

namespace render::asset {

struct Texture
{
  util::Uuid _id = util::Uuid::generate();

  explicit operator bool() const {
    return !_data.empty();
  }

  enum class Format : std::uint8_t
  {
    RGBA8_UNORM,
    RGBA8_SRGB,
    RGBA16F_SFLOAT
  } _format;

  std::vector<std::uint8_t> _data;
  unsigned _width;
  unsigned _height;
};

}