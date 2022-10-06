#pragma once

#include "Material.h"

namespace render {

class MaterialFactory
{
public:
  static Material createStandardMaterial(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);
  static Material createStandardInstancedMaterial(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);
};

}