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

    internal::InternalTexture internalTex{};
    internalTex._id = it->_id;

    auto res = createTexture(
      cmdBuf,
      uc,
      *it,
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
  render::asset::Texture& tex,
  VkSampler& samplerOut,
  AllocatedImage& imageOut,
  VkImageView& viewOut)
{
  auto& sb = uc->getStagingBuffer();
  auto format = imageutil::texFormatToVk(tex._format);

  // Make sure that we're at a multiple of 16
  auto mod = sb._currentOffset % 16;
  auto padding = 16 - mod;

  std::size_t dataSize = padding;
  for (auto& dat : tex._data) {
    dataSize += dat.size();
  }

  if (!sb.canFit(dataSize)) {
    return false;
  }

  AllocatedImage image;
  sb.advance(padding);

  imageutil::createImage(
    tex._width, tex._height,
    format,
    VK_IMAGE_TILING_OPTIMAL,
    uc->getRC()->vmaAllocator(),
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    image,
    tex._numMips);

  auto view = imageutil::createImageView(
    uc->getRC()->device(),
    image._image,
    format,
    VK_IMAGE_ASPECT_COLOR_BIT,
    0,
    tex._numMips);

  // Transition image to transfer dst
  imageutil::transitionImageLayout(
    cmdBuffer,
    image._image,
    format,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    0,
    tex._numMips);
  
  void* data;
  glm::uint8_t* mappedData = nullptr;
  vmaMapMemory(uc->getRC()->vmaAllocator(), sb._buf._allocation, &data);
  mappedData = (glm::uint8_t*)data;
  mappedData = mappedData + sb._currentOffset;

  // Copy each mip to staging buffer and then to image
  uint32_t mipWidth = tex._width;
  uint32_t mipHeight = tex._height;
  std::size_t rollingOffset = 0;
  for (unsigned i = 0; i < tex._numMips; ++i) {
    auto* data = mappedData + rollingOffset;

    std::memcpy(data, tex._data[i].data(), tex._data[i].size());

    VkExtent3D extent{};
    extent.depth = 1;
    extent.height = mipHeight;
    extent.width = mipWidth;

    VkOffset3D offset{};

    VkBufferImageCopy imCopy{};
    imCopy.bufferOffset = sb._currentOffset + rollingOffset;
    imCopy.imageExtent = extent;
    imCopy.imageOffset = offset;
    imCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imCopy.imageSubresource.layerCount = 1;
    imCopy.imageSubresource.mipLevel = i;

    vkCmdCopyBufferToImage(
      cmdBuffer,
      sb._buf._buffer,
      image._image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &imCopy);

    rollingOffset += tex._data[i].size();
    if (mipWidth > 1) mipWidth /= 2;
    if (mipHeight > 1) mipHeight /= 2;
  }

  // Transition image to shader
  imageutil::transitionImageLayout(
    cmdBuffer,
    image._image,
    format,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    0,
    tex._numMips);

  vmaUnmapMemory(uc->getRC()->vmaAllocator(), sb._buf._allocation);
  sb.advance(dataSize);

  SamplerCreateParams params{};
  params.renderContext = uc->getRC();

  imageOut = image;
  viewOut = view;
  samplerOut = createSampler(params);

  return true;
}

}