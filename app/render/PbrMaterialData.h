#pragma once

#include "AllocatedImage.h"

#include <string>
#include <vulkan/vulkan.h>

namespace render
{

struct PbrMaterialData
{
  std::string _texPath;

  int _bindlessIndex;

  AllocatedImage _image;
  VkImageView _view;
  VkSampler _sampler;
};

}