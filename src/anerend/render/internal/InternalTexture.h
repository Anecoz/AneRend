#pragma once

#include "../../util/Uuid.h"
#include "../AllocatedImage.h"
#include "BufferMemoryInterface.h"

#include <vulkan/vulkan.h>

namespace render::internal {

struct InternalTexture
{
  struct BindlessTexInfo
  {
    explicit operator bool() const { return !!_bindlessIndexHandle; }

    // Index into bindless descriptor.
    BufferMemoryInterface::Handle _bindlessIndexHandle;
    AllocatedImage _image;
    VkImageView _view;
    VkSampler _sampler;
  } _bindlessInfo;

  util::Uuid _id;
};

}