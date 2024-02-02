#include "GrassGenPass.h"

#include "../BufferHelpers.h"

#include "../RenderResource.h"
#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"

namespace render {

GrassGenPass::GrassGenPass()
  : RenderPass()
{}

GrassGenPass::~GrassGenPass()
{}

void GrassGenPass::cleanup(RenderContext* renderContext, RenderResourceVault*)
{
  for (auto& buf : _gpuStagingBuffers) {
    vmaDestroyBuffer(renderContext->vmaAllocator(), buf._buffer, buf._allocation);
  }
}

void GrassGenPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  // Create staging buffers
  _gpuStagingBuffers.resize(rc->getMultiBufferSize());
  auto allocator = rc->vmaAllocator();
  for (int i = 0; i < rc->getMultiBufferSize(); ++i) {
    bufferutil::createBuffer(
      allocator,
      sizeof(gpu::GPUNonIndexDrawCallCmd),
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
      _gpuStagingBuffers[i]);
  }

  // Add an initializer for the grass draw buffer
  ResourceUsage grassDrawBufInitUsage{};
  grassDrawBufInitUsage._type = Type::SSBO;
  grassDrawBufInitUsage._access.set((std::size_t)Access::Write);
  grassDrawBufInitUsage._stage.set((std::size_t)Stage::Transfer);

  fgb.registerResourceInitExe("CullGrassDrawBuf", std::move(grassDrawBufInitUsage),
    [this](IRenderResource* resource, VkCommandBuffer& cmdBuffer, RenderContext* renderContext) {
      auto buf = (BufferRenderResource*)resource;

      std::size_t dataSize = sizeof(gpu::GPUNonIndexDrawCallCmd);

      /*if (_currentStagingOffset + dataSize >= renderContext->getMaxNumRenderables() * sizeof(gpu::GPURenderable) * 2) {
        _currentStagingOffset = 0;
      }*/

      uint8_t* data;
      vmaMapMemory(renderContext->vmaAllocator(), _gpuStagingBuffers[renderContext->getCurrentMultiBufferIdx()]._allocation, (void**)&data);
      //data += _currentStagingOffset;

      gpu::GPUNonIndexDrawCallCmd* mappedData = reinterpret_cast<gpu::GPUNonIndexDrawCallCmd*>(data);

      mappedData->_command.firstInstance = 0;
      mappedData->_command.firstVertex = 0;
      mappedData->_command.instanceCount = 0;
      mappedData->_command.vertexCount = 15;

      VkBufferCopy copyRegion{};
      copyRegion.srcOffset = 0;// _currentStagingOffset;
      copyRegion.dstOffset = 0;
      copyRegion.size = sizeof(gpu::GPUNonIndexDrawCallCmd);
      vkCmdCopyBuffer(cmdBuffer, _gpuStagingBuffers[renderContext->getCurrentMultiBufferIdx()]._buffer, buf->_buffer._buffer, 1, &copyRegion);

      vmaUnmapMemory(renderContext->vmaAllocator(), _gpuStagingBuffers[renderContext->getCurrentMultiBufferIdx()]._allocation);
      //_currentStagingOffset += sizeof(gpu::GPUNonIndexDrawCallCmd);
    });

  RenderPassRegisterInfo info{};
  info._name = "GrassGen";

  // Draw command buffer
  {
    ResourceUsage grassDrawBufUsage{};
    grassDrawBufUsage._resourceName = "CullGrassDrawBuf";
    grassDrawBufUsage._access.set((std::size_t)Access::Write);
    grassDrawBufUsage._stage.set((std::size_t)Stage::Compute);
    grassDrawBufUsage._type = Type::SSBO;
    BufferInitialCreateInfo grassDrawCreateInfo{};
    grassDrawCreateInfo._multiBuffered = true;
    grassDrawCreateInfo._initialSize = sizeof(gpu::GPUNonIndexDrawCallCmd);
    grassDrawBufUsage._bufferCreateInfo = grassDrawCreateInfo;
    info._resourceUsages.emplace_back(std::move(grassDrawBufUsage));
  }

  // Object buffer for the grass blade data
  {
    ResourceUsage grassObjBufUsage{};
    grassObjBufUsage._resourceName = "CullGrassObjBuf";
    grassObjBufUsage._access.set((std::size_t)Access::Write);
    grassObjBufUsage._stage.set((std::size_t)Stage::Compute);
    grassObjBufUsage._type = Type::SSBO;
    BufferInitialCreateInfo grassObjCreateInfo{};
    grassObjCreateInfo._multiBuffered = true;
    grassObjCreateInfo._initialSize = MAX_NUM_GRASS_BLADES * sizeof(gpu::GPUGrassBlade);
    grassObjBufUsage._bufferCreateInfo = grassObjCreateInfo;
    info._resourceUsages.emplace_back(std::move(grassObjBufUsage));
  }

  // HiZ images used for culling
  int currSize = 64;
  for (int i = 0; i < 6; ++i) {
    ResourceUsage depthUsage{};
    depthUsage._resourceName = "HiZ" + std::to_string(currSize);
    depthUsage._access.set((std::size_t)Access::Read);
    depthUsage._stage.set((std::size_t)Stage::Compute);
    depthUsage._multiBuffered = true; // TODO: This is not true, it isn't multi buffered.. but we have to specify since all the other buffers are
    depthUsage._useMaxSampler = true;
    depthUsage._arrayId = 0;
    depthUsage._arrayIdx = i;
    depthUsage._type = Type::SampledTexture;
    info._resourceUsages.emplace_back(std::move(depthUsage));
    currSize = currSize >> 1;
  }

  ComputePipelineCreateParams param{};
  param.device = rc->device();
  param.shader = "grass_gen_comp.spv";
  info._computeParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("GrassGen",
    [this](RenderExeParams exeParams) {

      auto terrainIndices = exeParams.rc->getTerrainIndices();
      if (terrainIndices.empty()) return;

      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

      // Bind to set 1
      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[exeParams.rc->getCurrentMultiBufferIdx()],
        0, nullptr);

      auto pushConstants = exeParams.rc->getCullParams();
      vkCmdPushConstants(
        *exeParams.cmdBuffer,
        *exeParams.pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        0,
        sizeof(gpu::GPUCullPushConstants),
        &pushConstants);

      uint32_t localSize = 8;
      uint32_t numX = 256 / localSize;
      uint32_t numY = 256 / localSize;
      uint32_t numZ = (uint32_t)terrainIndices.back() + 1;

      vkCmdDispatch(*exeParams.cmdBuffer, numX, numY, numZ);
    });
}

}