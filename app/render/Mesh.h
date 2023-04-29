#pragma once

#include <cstddef>

#include "AllocatedImage.h"
#include "MeshId.h"

#include <vulkan/vulkan.h>

namespace render {

struct Mesh {
  MeshId _id;

  // Offsets into the fat mesh buffer
  std::size_t _vertexOffset;
  std::size_t _indexOffset;

  std::size_t _numVertices;
  std::size_t _numIndices;

  int _metallicTexIndex;
  int _roughnessTexIndex;
  int _normalTexIndex;
  int _albedoTexIndex;

  VkSampler _metallicSampler;
  AllocatedImage _metallicImage;
  VkImageView _metallicView;

  VkSampler _roughnessSampler;
  AllocatedImage _roughnessImage;
  VkImageView _roughnessView;

  VkSampler _normalSampler;
  AllocatedImage _normalImage;
  VkImageView _normalView;

  VkSampler _albedoSampler;
  AllocatedImage _albedoImage;
  VkImageView _albedoView;
};

}