#pragma once

#include "../AllocatedImage.h"
#include "../Identifiers.h"
#include "BufferMemoryInterface.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace render::internal {

struct InternalMaterial
{
  struct BindlessTexInfo
  {
    explicit operator bool() const { return !!_bindlessIndexHandle; }

    // Index into bindless descriptor.
    BufferMemoryInterface::Handle _bindlessIndexHandle;
    AllocatedImage _image;
    VkImageView _view;
    VkSampler _sampler;
  };

  render::MaterialId _id;

  BindlessTexInfo _albedoInfo;
  BindlessTexInfo _metRoughInfo;
  BindlessTexInfo _normalInfo;
  BindlessTexInfo _emissiveInfo;

  glm::vec3 _baseColFactor;
  glm::vec4 _emissive;
};

}