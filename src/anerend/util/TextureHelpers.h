#pragma once

#include "../render/asset/Texture.h"

#include <cstdint>
#include <vector>

namespace util {

struct TextureHelpers
{

  // 1 byte per channel
  static std::vector<std::uint8_t> convertRGBA8ToRGB8(std::vector<std::uint8_t> in);

  // 1 byte per channel, channels {1, 2} means GB to RG
  static std::vector<std::uint8_t> convertRGBA8ToRG8(std::vector<std::uint8_t> in, std::vector<unsigned> channels = { 1, 2 });

  // Output will also be 4 component
  static void convertRGBA8ToBC7(render::asset::Texture& tex);

  // Output will be 2 component
  static void convertRG8ToBC5(render::asset::Texture& tex);

};

}