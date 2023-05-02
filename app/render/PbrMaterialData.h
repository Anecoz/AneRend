#pragma once

#include "AllocatedImage.h"

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace render
{

struct PbrMaterialData
{
  std::string _texPath;

  int _bindlessIndex;

  std::vector<uint8_t> _data;
  int _width;
  int _height;

  AllocatedImage _image;
  VkImageView _view;
  VkSampler _sampler;
};

}