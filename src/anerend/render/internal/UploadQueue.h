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
    VkFormat format,
    int width,
    int height,
    const std::vector<uint8_t>& data,
    VkSampler& samplerOut,
    AllocatedImage& imageOut,
    VkImageView& viewOut);

  void generateMipmaps(
    VkCommandBuffer cmdBuffer,
    UploadContext* uc,
    VkImage image,
    VkFormat imageFormat,
    int32_t texWidth,
    int32_t texHeight,
    uint32_t mipLevels);
};

}