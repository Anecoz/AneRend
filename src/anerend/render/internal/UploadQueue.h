#pragma once

#include "../asset/Model.h"
#include "../asset/Texture.h"

#include "../AllocatedImage.h"

#include <vulkan/vulkan.h>

namespace render::internal {

class UploadContext;

class UploadQueue
{
public:
  UploadQueue();
  ~UploadQueue();

  UploadQueue(const UploadQueue&) = delete;
  UploadQueue(UploadQueue&&) = delete;
  UploadQueue& operator=(const UploadQueue&) = delete;
  UploadQueue& operator=(UploadQueue&&) = delete;

  void add(asset::Model modelToUpload);
  void add(asset::Texture textureToUpload);

  void execute(UploadContext* uc, VkCommandBuffer cmdBuf);

private:
  struct ModelUploadInfo
  {
    asset::Model _model;
    std::size_t _currentMeshIndex = 0;
  };

  std::vector<ModelUploadInfo> _modelsToUpload;
  std::vector<asset::Texture> _texturesToUpload;

  void uploadTextures(UploadContext* uc, VkCommandBuffer cmdBuf);
  void uploadModels(UploadContext* uc, VkCommandBuffer cmdBuf);

  bool createTexture(
    VkCommandBuffer cmdBuffer,
    UploadContext* uc,
    render::asset::Texture& tex,
    VkSampler& samplerOut,
    AllocatedImage& imageOut,
    VkImageView& viewOut);
};

}