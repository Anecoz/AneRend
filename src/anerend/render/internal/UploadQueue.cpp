#include "UploadQueue.h"

#include "../ImageHelpers.h"
#include "../PipelineUtil.h"
#include "UploadContext.h"

namespace render::internal {

UploadQueue::UploadQueue()
{}

UploadQueue::~UploadQueue()
{}

void UploadQueue::add(asset::Model modelToUpload)
{
  ModelUploadInfo info{};
  info._model = std::move(modelToUpload);
  _modelsToUpload.emplace_back(std::move(info));
}

void UploadQueue::add(asset::Texture textureToUpload)
{
  _texturesToUpload.emplace_back(std::move(textureToUpload));
}

void UploadQueue::execute(UploadContext* uc, VkCommandBuffer cmdBuf)
{
  uploadModels(uc, cmdBuf);
  uploadTextures(uc, cmdBuf);
}

void UploadQueue::uploadTextures(UploadContext* uc, VkCommandBuffer cmdBuf)
{
  for (auto it = _texturesToUpload.begin(); it != _texturesToUpload.end();) {
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    if (it->_format == asset::Texture::Format::RGBA8_SRGB) {
      format = VK_FORMAT_R8G8B8A8_SRGB;
    }
    else if (it->_format == asset::Texture::Format::RGBA16F_SFLOAT) {
      format = VK_FORMAT_R16G16B16A16_SFLOAT;
    }

    internal::InternalTexture internalTex{};
    internalTex._id = it->_id;

    auto res = createTexture(
      cmdBuf,
      uc,
      format,
      it->_width,
      it->_height,
      it->_data,
      internalTex._bindlessInfo._sampler,
      internalTex._bindlessInfo._image,
      internalTex._bindlessInfo._view);

    // Stop, because staging buffer is full (or something went very wrong?)
    if (!res) {
      break;
    }

    it = _texturesToUpload.erase(it);

    uc->textureUploadedCB(std::move(internalTex));
  }
}

void UploadQueue::uploadModels(UploadContext* uc, VkCommandBuffer cmdBuf)
{
  auto& sb = uc->getStagingBuffer();
  bool full = false;

  for (auto it = _modelsToUpload.begin(); it != _modelsToUpload.end();) {

    auto& model = it->_model;

    for (auto meshIt = model._meshes.begin() + it->_currentMeshIndex; meshIt != model._meshes.end(); ++meshIt) {
      auto& mesh = *meshIt;

      std::size_t vertSize = mesh._vertices.size() * sizeof(Vertex);
      std::size_t indSize = mesh._indices.size() * sizeof(std::uint32_t);
      std::size_t stagingBufSize = vertSize + indSize;

      if (!sb.canFit(stagingBufSize)) {
        full = true;
        break;
      }

      uint8_t* data = nullptr;
      vmaMapMemory(uc->getRC()->vmaAllocator(), sb._buf._allocation, (void**)&data);

      // Offset according to current staging buffer usage
      data = data + sb._currentOffset;

      internal::BufferMemoryInterface::Handle vertexHandle;
      internal::BufferMemoryInterface::Handle indexHandle;

      // Vertices first
      {
        // Find where to copy data in the fat buffer
        vertexHandle = uc->getVtxBuffer()._memInterface.addData(vertSize);

        if (!vertexHandle) {
          printf("Could not add %zu bytes of vertex data! Make buffer bigger! Things won't work now!\n", vertSize);
          continue;
        }

        memcpy(data, mesh._vertices.data(), vertSize);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = sb._currentOffset;
        copyRegion.dstOffset = vertexHandle._offset;
        copyRegion.size = vertSize;
        vkCmdCopyBuffer(cmdBuf, sb._buf._buffer, uc->getVtxBuffer()._buffer._buffer, 1, &copyRegion);

        VkBufferMemoryBarrier memBarr{};
        memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memBarr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memBarr.buffer = uc->getVtxBuffer()._buffer._buffer;
        memBarr.offset = vertexHandle._offset;
        memBarr.size = vertSize;

        vkCmdPipelineBarrier(
          cmdBuf,
          VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
          0, 0, nullptr,
          1, &memBarr,
          0, nullptr);
      }

      // Now indices
      if (indSize > 0) {
        // Find where to copy data in the fat buffer
        indexHandle = uc->getIdxBuffer()._memInterface.addData(indSize);

        if (!indexHandle) {
          printf("Could not add %zu bytes of index data! Make buffer bigger! Things won't work now!\n", indSize);
          continue;
        }

        memcpy(data + vertSize, mesh._indices.data(), indSize);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = sb._currentOffset + vertSize;
        copyRegion.dstOffset = indexHandle._offset;
        copyRegion.size = indSize;
        vkCmdCopyBuffer(cmdBuf, sb._buf._buffer, uc->getIdxBuffer()._buffer._buffer, 1, &copyRegion);

        VkBufferMemoryBarrier memBarr{};
        memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memBarr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_INDEX_READ_BIT;
        memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memBarr.buffer = uc->getIdxBuffer()._buffer._buffer;
        memBarr.offset = indexHandle._offset;
        memBarr.size = indSize;

        vkCmdPipelineBarrier(
          cmdBuf,
          VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
          0, 0, nullptr,
          1, &memBarr,
          0, nullptr);
      }

      it->_currentMeshIndex = meshIt - model._meshes.begin() + 1;

      internal::InternalMesh internalMesh{};
      internalMesh._id = mesh._id;
      internalMesh._numIndices = static_cast<uint32_t>(mesh._indices.size());
      internalMesh._numVertices = static_cast<uint32_t>(mesh._vertices.size());
      internalMesh._minPos = mesh._minPos;
      internalMesh._maxPos = mesh._maxPos;
      internalMesh._vertexHandle = vertexHandle;
      internalMesh._indexHandle = indexHandle;
      internalMesh._vertexOffset = static_cast<uint32_t>(vertexHandle._offset / sizeof(Vertex));
      internalMesh._indexOffset = indexHandle._offset == -1 ? -1 : static_cast<int64_t>(indexHandle._offset / sizeof(uint32_t));

      sb.advance(stagingBufSize);
      vmaUnmapMemory(uc->getRC()->vmaAllocator(), sb._buf._allocation);

      uc->modelMeshUploadedCB(std::move(internalMesh), cmdBuf);
    }

    if (full) {
      break;
    }

    it = _modelsToUpload.erase(it);
  }
}

bool UploadQueue::createTexture(
  VkCommandBuffer cmdBuffer,
  UploadContext* uc,
  VkFormat format,
  int width,
  int height,
  const std::vector<uint8_t>& data,
  VkSampler& samplerOut,
  AllocatedImage& imageOut,
  VkImageView& viewOut)
{
  auto& sb = uc->getStagingBuffer();

  // Make sure that we're at a multiple of 8
  auto mod = sb._currentOffset % 8;
  auto padding = 8 - mod;

  if (!sb.canFit(data.size() + padding)) {
    return false;
  }

  uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
  AllocatedImage image;

  imageutil::createImage(
    width, height,
    format,
    VK_IMAGE_TILING_OPTIMAL,
    uc->getRC()->vmaAllocator(),
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    image,
    mipLevels);

  auto view = imageutil::createImageView(
    uc->getRC()->device(),
    image._image,
    format,
    VK_IMAGE_ASPECT_COLOR_BIT,
    0,
    mipLevels);

  std::size_t dataSize = data.size() + padding;

  glm::uint8_t* mappedData;
  vmaMapMemory(uc->getRC()->vmaAllocator(), sb._buf._allocation, &(void*)mappedData);
  // Offset according to current staging buffer usage
  mappedData = mappedData + sb._currentOffset + padding;
  std::memcpy(mappedData, data.data(), data.size());
  vmaUnmapMemory(uc->getRC()->vmaAllocator(), sb._buf._allocation);

  // Transition image to dst optimal
  imageutil::transitionImageLayout(
    cmdBuffer,
    image._image,
    format,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    0,
    mipLevels);

  VkExtent3D extent{};
  extent.depth = 1;
  extent.height = height;
  extent.width = width;

  VkOffset3D offset{};

  VkBufferImageCopy imCopy{};
  imCopy.bufferOffset = sb._currentOffset + padding;
  imCopy.imageExtent = extent;
  imCopy.imageOffset = offset;
  imCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imCopy.imageSubresource.layerCount = 1;

  vkCmdCopyBufferToImage(
    cmdBuffer,
    sb._buf._buffer,
    image._image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &imCopy);

  sb.advance(dataSize);

  generateMipmaps(cmdBuffer, uc, image._image, format, width, height, mipLevels);

  SamplerCreateParams params{};
  params.renderContext = uc->getRC();

  imageOut = image;
  viewOut = view;
  samplerOut = createSampler(params);

  return true;
}

void UploadQueue::generateMipmaps(
  VkCommandBuffer cmdBuffer, 
  UploadContext* uc,
  VkImage image, 
  VkFormat imageFormat, 
  int32_t texWidth, 
  int32_t texHeight, 
  uint32_t mipLevels)
{
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  int32_t mipWidth = texWidth;
  int32_t mipHeight = texHeight;

  for (uint32_t i = 1; i < mipLevels; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
      0, nullptr,
      0, nullptr,
      1, &barrier);

    VkImageBlit blit{};
    blit.srcOffsets[0] = { 0, 0, 0 };
    blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = { 0, 0, 0 };
    blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(cmdBuffer,
      image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1, &blit,
      VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
      0, nullptr,
      0, nullptr,
      1, &barrier);

    if (mipWidth > 1) mipWidth /= 2;
    if (mipHeight > 1) mipHeight /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(cmdBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
    0, nullptr,
    0, nullptr,
    1, &barrier);
}


}