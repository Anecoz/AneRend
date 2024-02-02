#include "ImageManipulator.h"

#include <render/ImageHelpers.h>

namespace tool {

namespace {

inline void writeOrKeep(std::uint8_t& old, std::uint8_t val)
{
  if (val > old) old = val;
}

}

ImageManipulator::ImageManipulator(Brush brush)
  : _brush(std::move(brush))
{}

#if 0
void ImageManipulator::paintRGBA8(
  render::asset::Texture& texture,
  unsigned x, unsigned y,
  const glm::u8vec4& value,
  PaintMask mask,
  bool keepMax)
{
  if (texture._format != render::asset::Texture::Format::RGBA8_UNORM) {
    printf("ImageManipulator paintRGBA8 needs 8bit texture to work!\n");
    return;
  }

  if (texture._data.empty()) {
    printf("No data in texture, ImageManipulator cannot work\n");
  }

  auto w = texture._width;
  auto h = texture._height;
  auto s = render::imageutil::stride(texture._format);
  auto dims = render::imageutil::numDimensions(texture._format);

  auto& data = texture._data[0];

  // Check that we can fit given current position and brush
  glm::vec2 center((float)x, (float)y);
  float thresholdDist = (float)_brush._radius - (float)_brush._falloff;

  for (unsigned i = x - _brush._radius; i < x + _brush._radius; ++i) {
    for (unsigned j = y - _brush._radius; j < y + _brush._radius; ++j) {

      if (i > texture._width - 1 || j > texture._height - 1 ||
          i < 0 || j < 0) {
        continue;
      }

      glm::vec2 coord((float)i, (float)j);
      float dist = glm::distance(coord, center);
      if (dist >= _brush._radius) {
        continue;
      }

      float falloff = 1.0f;

      if (dist >= thresholdDist) {
        float aboveFalloff = dist - thresholdDist;
        falloff = 1.0f - aboveFalloff / (float)_brush._falloff;
      }

      if (keepMax) {
        if (dims > 0 && mask._r) writeOrKeep(data[s * (w * j + i) + 0], (std::uint8_t)(value[0] * falloff));
        if (dims > 1 && mask._g) writeOrKeep(data[s * (w * j + i) + 1], (std::uint8_t)(value[1] * falloff));
        if (dims > 2 && mask._b) writeOrKeep(data[s * (w * j + i) + 2], (std::uint8_t)(value[2] * falloff));
        if (dims > 3 && mask._a) writeOrKeep(data[s * (w * j + i) + 3], (std::uint8_t)(value[3] * falloff));
      }
      else {
        if (dims > 0 && mask._r) data[s * (w * j + i) + 0] = (std::uint8_t)(value[0] * falloff);
        if (dims > 1 && mask._g) data[s * (w * j + i) + 1] = (std::uint8_t)(value[1] * falloff);
        if (dims > 2 && mask._b) data[s * (w * j + i) + 2] = (std::uint8_t)(value[2] * falloff);
        if (dims > 3 && mask._a) data[s * (w * j + i) + 3] = (std::uint8_t)(value[3] * falloff);
      }
    }
  }
}

#endif

void ImageManipulator::paintRGBA8(render::asset::Texture& texture, int x, int y, std::function<void(glm::u8vec4*, float)> cb)
{
  if (texture._format != render::asset::Texture::Format::RGBA8_UNORM) {
    printf("ImageManipulator paintRGBA8 needs 8bit texture to work!\n");
    return;
  }

  if (texture._data.empty()) {
    printf("No data in texture, ImageManipulator cannot work\n");
  }

  auto w = texture._width;
  auto h = texture._height;
  auto s = render::imageutil::stride(texture._format);
  auto dims = render::imageutil::numDimensions(texture._format);

  auto& data = texture._data[0];

  // Check that we can fit given current position and brush
  glm::vec2 center((float)x, (float)y);
  float thresholdDist = (float)_brush._radius - (float)_brush._falloff;

  for (int i = x - (int)_brush._radius; i < x + (int)_brush._radius; ++i) {
    for (int j = y - (int)_brush._radius; j < y + (int)_brush._radius; ++j) {

      if (i > (int)texture._width - 1 || j > (int)texture._height - 1 ||
        i < 0 || j < 0) {
        continue;
      }

      glm::vec2 coord((float)i, (float)j);
      float dist = glm::distance(coord, center);
      if (dist >= _brush._radius) {
        continue;
      }

      float falloff = 1.0f;

      if (dist >= thresholdDist) {
        float aboveFalloff = dist - thresholdDist;
        falloff = 1.0f - aboveFalloff / (float)_brush._falloff;
      }

      glm::u8vec4* oldval = reinterpret_cast<glm::u8vec4*>(data.data() + s * (w * j + i) + 0);
      cb(oldval, falloff);
    }
  }
}

void ImageManipulator::paintR8(render::asset::Texture& texture, int x, int y, std::function<void(std::uint8_t*, float)> cb)
{
  if (texture._format != render::asset::Texture::Format::R8_UNORM) {
    printf("ImageManipulator paintR8 needs 8bit texture to work!\n");
    return;
  }

  if (texture._data.empty()) {
    printf("No data in texture, ImageManipulator cannot work\n");
  }

  auto w = texture._width;
  auto h = texture._height;

  auto& data = texture._data[0];

  // Check that we can fit given current position and brush
  glm::vec2 center((float)x, (float)y);
  float thresholdDist = (float)_brush._radius - (float)_brush._falloff;

  for (int i = x - (int)_brush._radius; i < x + (int)_brush._radius; ++i) {
    for (int j = y - (int)_brush._radius; j < y + (int)_brush._radius; ++j) {

      if (i > (int)texture._width - 1 || j > (int)texture._height - 1 ||
        i < 0 || j < 0) {
        continue;
      }

      glm::vec2 coord((float)i, (float)j);
      float dist = glm::distance(coord, center);
      if (dist >= _brush._radius) {
        continue;
      }

      float falloff = 1.0f;

      if (dist >= thresholdDist) {
        float aboveFalloff = dist - thresholdDist;
        falloff = 1.0f - aboveFalloff / (float)_brush._falloff;
      }

      std::uint8_t* oldval = reinterpret_cast<std::uint8_t*>(data.data() + (w * j + i));
      cb(oldval, falloff);
    }
  }
}

}