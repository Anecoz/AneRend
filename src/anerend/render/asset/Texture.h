#pragma once

#include "../../util/Uuid.h"

#include <cstdint>
#include <vector>

namespace render::asset {

struct Texture
{
  util::Uuid _id = util::Uuid::generate();

  std::string _name;

  explicit operator bool() const {
    return !_data.empty();
  }

  enum class Format : std::uint8_t
  {
    RGBA8_UNORM,
    RGBA8_SRGB,
    RGB8_SRGB,
    RGB8_UNORM,
    RG8_UNORM,
    RGBA16F_SFLOAT,
    RGBA_SRGB_BC7,
    RGBA_UNORM_BC7,
    RG_UNORM_BC5,
    R8_UNORM,
    R16_UNORM
  } _format;

  unsigned _numMips = 1; // Needed so we know how to deserialise
  std::vector<std::vector<std::uint8_t>> _data; // One entry for each mip, _data[0] is most hi-res and so on
  unsigned _width;
  unsigned _height;

  bool _clampToEdge = false; // TODO: This is a sampling thing, not a texture attribute...
};

}