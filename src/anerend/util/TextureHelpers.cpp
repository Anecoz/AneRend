#include "TextureHelpers.h"

#include "../render/ImageHelpers.h"

#include <ktx.h>
#include <vulkan/vulkan.h>

namespace util {

namespace {

KTX_error_code fillMipsOfTex(
  int miplevel, int face,
  int width, int height, int depth,
  ktx_uint64_t faceLodSize,
  void* pixels, void* userdata)
{
  auto* tex = (render::asset::Texture*)userdata;
  tex->_data[miplevel].resize(faceLodSize);
  std::memcpy(tex->_data[miplevel].data(), pixels, faceLodSize);

  return KTX_SUCCESS;
}

}

render::asset::Texture TextureHelpers::createTextureRGBA8(unsigned w, unsigned h, glm::u8vec4 val)
{
  render::asset::Texture tex;

  tex._width = w;
  tex._height = h;
  tex._format = render::asset::Texture::Format::RGBA8_UNORM;
  tex._data.emplace_back();
  tex._data.back().resize(w * h * 4);

  auto& data = tex._data.back();
  for (unsigned x = 0; x < w; x++) {
    for (unsigned y = 0; y < h; y++) {
      data[4 * (w * y + x) + 0] = val[0];
      data[4 * (w * y + x) + 1] = val[1];
      data[4 * (w * y + x) + 2] = val[2];
      data[4 * (w * y + x) + 3] = val[3];
    }
  }

  //std::fill(tex._data.back().begin(), tex._data.back().end(), initialVal);

  return tex;
}

std::vector<std::uint8_t> TextureHelpers::convertRGBA8ToRGB8(std::vector<std::uint8_t> in)
{
  std::vector<std::uint8_t> out;
  out.resize(in.size() / 4 * 3);

  for (std::size_t i = 0; i < in.size() / 4; ++i) {
    out[i * 3 + 0] = in[i * 4 + 0];
    out[i * 3 + 1] = in[i * 4 + 1];
    out[i * 3 + 2] = in[i * 4 + 2];
  }

  return out;
}

std::vector<std::uint8_t> TextureHelpers::convertRGBA8ToRG8(std::vector<std::uint8_t> in, std::vector<unsigned> channels)
{
  std::vector<std::uint8_t> out;
  out.resize(in.size() / 4 * 2);

  for (std::size_t i = 0; i < in.size() / 4; ++i) {
    out[i * 2 + 0] = in[i * 4 + channels[0]];
    out[i * 2 + 1] = in[i * 4 + channels[1]];
  }

  return out;
}

void TextureHelpers::convertRGBA8ToBC7(render::asset::Texture& tex)
{
  ktxTexture2* texture;
  ktxTextureCreateInfo createInfo;
  KTX_error_code result;
  ktx_uint32_t level, layer, faceSlice;
  ktx_size_t srcSize;
  ktxBasisParams params = { 0 };
  params.structSize = sizeof(params);

  createInfo.glInternalformat = 0;  //Ignored as we'll create a KTX2 texture.
  createInfo.vkFormat = render::imageutil::texFormatToVk(tex._format);
  createInfo.baseWidth = tex._width;
  createInfo.baseHeight = tex._height;
  createInfo.baseDepth = 1;
  createInfo.numDimensions = std::min(render::imageutil::numDimensions(tex._format), 3u);
  createInfo.numLevels = tex._numMips;
  createInfo.numLayers = 1;
  createInfo.numFaces = 1;
  createInfo.isArray = KTX_FALSE;
  createInfo.generateMipmaps = KTX_FALSE;

  result = ktxTexture2_Create(&createInfo,
    KTX_TEXTURE_CREATE_ALLOC_STORAGE,
    &texture);

  if (result != KTX_SUCCESS) {
    printf("Could not create ktx image!\n");
    return;
  }

  layer = 0;
  faceSlice = 0;
  for (unsigned i = 0; i < tex._numMips; ++i) {
    srcSize = tex._data[i].size();
    level = i;

    result = ktxTexture_SetImageFromMemory(ktxTexture(texture), level, layer, faceSlice, tex._data[i].data(), srcSize);

    if (result != KTX_SUCCESS) {
      printf("Could not create ktx image!\n");
      return;
    }
  }

  // Compress I guess...
  // For BasisLZ/ETC1S
  params.compressionLevel = KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL;
  params.threadCount = 8;
  // For UASTC
  params.uastc = KTX_TRUE;
  // Set other BasisLZ/ETC1S or UASTC params to change default quality settings.
  result = ktxTexture2_CompressBasisEx(texture, &params);

  if (result != KTX_SUCCESS) {
    printf("Could not compress ktx!\n");
    return;
  }
  
  ktx_transcode_fmt_e targetFormat = KTX_TTF_BC7_RGBA;

  // Transcode
  if (ktxTexture2_NeedsTranscoding(texture)) {
    result = ktxTexture2_TranscodeBasis(texture, targetFormat, 0);
  }

  if (result != VK_SUCCESS) {
    printf("Could not transcode to bc7!\n");
    return;
  }

  // Copy data to output
  ktxTexture_IterateLevels(ktxTexture(texture), fillMipsOfTex, &tex);
  auto vkFormat = (VkFormat)texture->vkFormat;
  switch (vkFormat) {
  case VK_FORMAT_BC7_SRGB_BLOCK:
    tex._format = render::asset::Texture::Format::RGBA_SRGB_BC7;
    break;
  case VK_FORMAT_BC7_UNORM_BLOCK:
    tex._format = render::asset::Texture::Format::RGBA_UNORM_BC7;
    break;
  default:
    tex._format = render::asset::Texture::Format::RGBA_UNORM_BC7;
  }
}

void TextureHelpers::convertRG8ToBC5(render::asset::Texture& tex)
{
  ktxTexture2* texture;
  ktxTextureCreateInfo createInfo;
  KTX_error_code result;
  ktx_uint32_t level, layer, faceSlice;
  ktx_size_t srcSize;
  ktxBasisParams params = { 0 };
  params.structSize = sizeof(params);

  createInfo.glInternalformat = 0;  //Ignored as we'll create a KTX2 texture.
  createInfo.vkFormat = render::imageutil::texFormatToVk(tex._format);
  createInfo.baseWidth = tex._width;
  createInfo.baseHeight = tex._height;
  createInfo.baseDepth = 1;
  createInfo.numDimensions = std::min(render::imageutil::numDimensions(tex._format), 3u);
  createInfo.numLevels = tex._numMips;
  createInfo.numLayers = 1;
  createInfo.numFaces = 1;
  createInfo.isArray = KTX_FALSE;
  createInfo.generateMipmaps = KTX_FALSE;

  result = ktxTexture2_Create(&createInfo,
    KTX_TEXTURE_CREATE_ALLOC_STORAGE,
    &texture);

  if (result != KTX_SUCCESS) {
    printf("Could not create ktx image!\n");
    return;
  }

  layer = 0;
  faceSlice = 0;
  for (unsigned i = 0; i < tex._numMips; ++i) {
    srcSize = tex._data[i].size();
    level = i;

    result = ktxTexture_SetImageFromMemory(ktxTexture(texture), level, layer, faceSlice, tex._data[i].data(), srcSize);

    if (result != KTX_SUCCESS) {
      printf("Could not create ktx image!\n");
      return;
    }
  }

  // Compress I guess...
  // For BasisLZ/ETC1S
  params.compressionLevel = KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL;
  params.threadCount = 8;
  // For UASTC
  params.uastc = KTX_TRUE;
  params.inputSwizzle[0] = 'r';
  params.inputSwizzle[1] = 'r';
  params.inputSwizzle[2] = 'r';
  params.inputSwizzle[3] = 'g';
  // Set other BasisLZ/ETC1S or UASTC params to change default quality settings.
  result = ktxTexture2_CompressBasisEx(texture, &params);

  if (result != KTX_SUCCESS) {
    printf("Could not compress ktx!\n");
    return;
  }

  ktx_transcode_fmt_e targetFormat = KTX_TTF_BC5_RG;

  // Transcode
  if (ktxTexture2_NeedsTranscoding(texture)) {
    result = ktxTexture2_TranscodeBasis(texture, targetFormat, 0);
  }

  if (result != VK_SUCCESS) {
    printf("Could not transcode to bc7!\n");
    return;
  }

  // Copy data to output
  ktxTexture_IterateLevels(ktxTexture(texture), fillMipsOfTex, &tex);
  tex._format = render::asset::Texture::Format::RG_UNORM_BC5;
}

}