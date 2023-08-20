#include "CullRenderPass.h"

#include "../GpuBuffers.h"
#include "../BufferHelpers.h"
#include "../RenderResource.h"
#include "../RenderResourceVault.h"
#include "../FrameGraphBuilder.h"

namespace render {

CullRenderPass::CullRenderPass()
  : RenderPass()
{}

CullRenderPass::~CullRenderPass()
{
}

void CullRenderPass::cleanup(RenderContext* renderContext, RenderResourceVault*)
{
  for (auto& buf : _gpuStagingBuffers) {
    vmaDestroyBuffer(renderContext->vmaAllocator(), buf._buffer, buf._allocation);
  }
}

void CullRenderPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  // Create staging buffers
  _gpuStagingBuffers.resize(rc->getMultiBufferSize());
  auto allocator = rc->vmaAllocator();
  for (int i = 0; i < rc->getMultiBufferSize(); ++i) {
    bufferutil::createBuffer(
      allocator,
      rc->getMaxNumRenderables() * sizeof(gpu::GPURenderable) * 2, // We use this as the size, as the GPU buffers can never get bigger than this
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
      _gpuStagingBuffers[i]);
  }

  // Add an initializer for the draw buffer
  ResourceUsage drawBufInitUsage{};
  drawBufInitUsage._type = Type::SSBO;
  drawBufInitUsage._access.set((std::size_t)Access::Write);
  drawBufInitUsage._stage.set((std::size_t)Stage::Transfer);

  fgb.registerResourceInitExe("CullDrawBuf", std::move(drawBufInitUsage),
    [this](IRenderResource* resource, VkCommandBuffer& cmdBuffer, RenderContext* renderContext) {
      // Prefill with draw cmd data using rendercontext to get current mesh data
      auto buf = (BufferRenderResource*)resource;
      
      auto& meshes = renderContext->getCurrentMeshes();
      std::size_t dataSize = meshes.size() * sizeof(gpu::GPUDrawCallCmd);

      if (_currentStagingOffset + dataSize >= renderContext->getMaxNumRenderables() * sizeof(gpu::GPURenderable) * 2) {
        _currentStagingOffset = 0;
      }

      uint8_t* data;
      vmaMapMemory(renderContext->vmaAllocator(), _gpuStagingBuffers[renderContext->getCurrentMultiBufferIdx()]._allocation, (void**)&data);
      
      data += _currentStagingOffset;

      gpu::GPUDrawCallCmd* mappedData = reinterpret_cast<gpu::GPUDrawCallCmd*>(data);
      uint32_t cumulativeMeshUsage = 0;
      for (std::size_t i = 0; i < meshes.size(); ++i) {
        auto& mesh = meshes[i];

        mappedData[i]._command.indexCount = (uint32_t)mesh._numIndices;
        mappedData[i]._command.instanceCount = 0; // Updated by GPU compute shader
        mappedData[i]._command.firstIndex = (uint32_t)mesh._indexOffset;
        mappedData[i]._command.vertexOffset = (uint32_t)mesh._vertexOffset;
        mappedData[i]._command.firstInstance = cumulativeMeshUsage;//i == 0 ? 0 : (uint32_t)renderContext->getCurrentMeshUsage()[(uint32_t)i - 1];

        cumulativeMeshUsage += (uint32_t)renderContext->getCurrentMeshUsage()[(uint32_t)i];
      }

      VkBufferCopy copyRegion{};
      copyRegion.srcOffset = _currentStagingOffset;
      copyRegion.dstOffset = 0;
      copyRegion.size = dataSize;
      vkCmdCopyBuffer(cmdBuffer, _gpuStagingBuffers[renderContext->getCurrentMultiBufferIdx()]._buffer, buf->_buffer._buffer, 1, &copyRegion);

      vmaUnmapMemory(renderContext->vmaAllocator(), _gpuStagingBuffers[renderContext->getCurrentMultiBufferIdx()]._allocation);
      _currentStagingOffset += dataSize;
    });

  // Add an initializer for the trans buffer
  ResourceUsage transBufInitUsage{};
  transBufInitUsage._type = Type::SSBO;
  transBufInitUsage._access.set((std::size_t)Access::Write);
  transBufInitUsage._stage.set((std::size_t)Stage::Transfer);

  fgb.registerResourceInitExe("CullTransBuf", std::move(transBufInitUsage),
    [this](IRenderResource* resource, VkCommandBuffer& cmdBuffer, RenderContext* renderContext) {
      // Just fill buffer with 0's
      auto buf = (BufferRenderResource*)resource;

      vkCmdFillBuffer(
        cmdBuffer,
        buf->_buffer._buffer,
        0,
        VK_WHOLE_SIZE,
        0);
    });

  // Add an initializer for the grass draw buffer
  ResourceUsage grassDrawBufInitUsage{};
  grassDrawBufInitUsage._type = Type::SSBO;
  grassDrawBufInitUsage._access.set((std::size_t)Access::Write);
  grassDrawBufInitUsage._stage.set((std::size_t)Stage::Transfer);

  fgb.registerResourceInitExe("CullGrassDrawBuf", std::move(grassDrawBufInitUsage),
    [this](IRenderResource* resource, VkCommandBuffer& cmdBuffer, RenderContext* renderContext) {
      auto buf = (BufferRenderResource*)resource;

      std::size_t dataSize = sizeof(gpu::GPUNonIndexDrawCallCmd);

      if (_currentStagingOffset + dataSize >= renderContext->getMaxNumRenderables() * sizeof(gpu::GPURenderable) * 2) {
        _currentStagingOffset = 0;
      }

      uint8_t* data;
      vmaMapMemory(renderContext->vmaAllocator(), _gpuStagingBuffers[renderContext->getCurrentMultiBufferIdx()]._allocation, (void**)&data);
      data += _currentStagingOffset;

      gpu::GPUNonIndexDrawCallCmd* mappedData = reinterpret_cast<gpu::GPUNonIndexDrawCallCmd*>(data);

      mappedData->_command.firstInstance = 0;
      mappedData->_command.firstVertex = 0;
      mappedData->_command.instanceCount = 0;
      mappedData->_command.vertexCount = 15;

      VkBufferCopy copyRegion{};
      copyRegion.srcOffset = _currentStagingOffset;
      copyRegion.dstOffset = 0;
      copyRegion.size = sizeof(gpu::GPUNonIndexDrawCallCmd);
      vkCmdCopyBuffer(cmdBuffer, _gpuStagingBuffers[renderContext->getCurrentMultiBufferIdx()]._buffer, buf->_buffer._buffer, 1, &copyRegion);

      vmaUnmapMemory(renderContext->vmaAllocator(), _gpuStagingBuffers[renderContext->getCurrentMultiBufferIdx()]._allocation);
      _currentStagingOffset += sizeof(gpu::GPUNonIndexDrawCallCmd);
    });

  // An initializer for the bounding sphere visualizer draw buffer
  {
    ResourceUsage initUsage{};
    initUsage._type = Type::SSBO;
    initUsage._access.set((std::size_t)Access::Write);
    initUsage._stage.set((std::size_t)Stage::Transfer);

    fgb.registerResourceInitExe("CullBSDrawBuf", std::move(initUsage),
      [this](IRenderResource* resource, VkCommandBuffer& cmdBuffer, RenderContext* renderContext) {
        auto buf = (BufferRenderResource*)resource;

        std::size_t dataSize = sizeof(gpu::GPUDrawCallCmd);

        if (_currentStagingOffset + dataSize >= renderContext->getMaxNumRenderables() * sizeof(gpu::GPURenderable) * 2) {
          _currentStagingOffset = 0;
        }

        uint8_t* data;
        vmaMapMemory(renderContext->vmaAllocator(), _gpuStagingBuffers[renderContext->getCurrentMultiBufferIdx()]._allocation, (void**)&data);
        data += _currentStagingOffset;

        auto& mesh = renderContext->getSphereMesh();

        gpu::GPUDrawCallCmd* mappedData = reinterpret_cast<gpu::GPUDrawCallCmd*>(data);

        mappedData->_command.indexCount = (uint32_t)mesh._numIndices;
        mappedData->_command.instanceCount = 0; // Updated by GPU compute shader
        mappedData->_command.firstIndex = (uint32_t)mesh._indexOffset;
        mappedData->_command.vertexOffset = (uint32_t)mesh._vertexOffset;
        mappedData->_command.firstInstance = 0;

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = _currentStagingOffset;
        copyRegion.dstOffset = 0;
        copyRegion.size = dataSize;
        vkCmdCopyBuffer(cmdBuffer, _gpuStagingBuffers[renderContext->getCurrentMultiBufferIdx()]._buffer, buf->_buffer._buffer, 1, &copyRegion);

        vmaUnmapMemory(renderContext->vmaAllocator(), _gpuStagingBuffers[renderContext->getCurrentMultiBufferIdx()]._allocation);
        _currentStagingOffset += dataSize;
      });
  }

  // Add an initializer for bounding sphere translation buffer
  {
    ResourceUsage initUsage{};
    initUsage._type = Type::SSBO;
    initUsage._access.set((std::size_t)Access::Write);
    initUsage._stage.set((std::size_t)Stage::Transfer);

    fgb.registerResourceInitExe("CullBSTransBuf", std::move(initUsage),
      [this](IRenderResource* resource, VkCommandBuffer& cmdBuffer, RenderContext* renderContext) {
        // Just fill buffer with 0's
        auto buf = (BufferRenderResource*)resource;

        vkCmdFillBuffer(
          cmdBuffer,
          buf->_buffer._buffer,
          0,
          VK_WHOLE_SIZE,
          0);
      });
  }

  // Add an initializer for the grass obj buffer
  /*ResourceUsage grassObjBufInitUsage{};
  grassObjBufInitUsage._type = Type::SSBO;
  grassObjBufInitUsage._access.set((std::size_t)Access::Write);
  grassObjBufInitUsage._stage.set((std::size_t)Stage::Transfer);

  fgb.registerResourceInitExe("CullGrassObjBuf", std::move(grassObjBufInitUsage),
    [this](IRenderResource* resource, VkCommandBuffer& cmdBuffer, RenderContext* renderContext) {
      // Just fill buffer with 0's
      auto buf = (BufferRenderResource*)resource;

      vkCmdFillBuffer(
        cmdBuffer,
        buf->_buffer._buffer,
        0,
        VK_WHOLE_SIZE,
        0);
    });*/

  // Register the actual render pass
  RenderPassRegisterInfo regInfo{};
  regInfo._name = "Cull";
  
  ResourceUsage drawCmdUsage{};
  drawCmdUsage._resourceName = "CullDrawBuf";
  drawCmdUsage._access.set((std::size_t)Access::Write);
  drawCmdUsage._access.set((std::size_t)Access::Read);
  drawCmdUsage._stage.set((std::size_t)Stage::Compute);
  drawCmdUsage._type = Type::SSBO;
  BufferInitialCreateInfo drawCreateInfo{};
  drawCreateInfo._multiBuffered = true;
  drawCreateInfo._initialSize = rc->getMaxNumMeshes() * sizeof(gpu::GPUDrawCallCmd);
  drawCmdUsage._bufferCreateInfo = drawCreateInfo;
  regInfo._resourceUsages.emplace_back(std::move(drawCmdUsage));

  ResourceUsage translationBufUsage{};
  translationBufUsage._resourceName = "CullTransBuf";
  translationBufUsage._access.set((std::size_t)Access::Write);
  translationBufUsage._access.set((std::size_t)Access::Read);
  translationBufUsage._stage.set((std::size_t)Stage::Compute);
  translationBufUsage._type = Type::SSBO;
  BufferInitialCreateInfo transCreateInfo{};
  transCreateInfo._multiBuffered = true;
  transCreateInfo._initialSize = rc->getMaxNumRenderables() * sizeof(gpu::GPUTranslationId);
  translationBufUsage._bufferCreateInfo = transCreateInfo;
  regInfo._resourceUsages.emplace_back(std::move(translationBufUsage));

  ResourceUsage grassDrawBufUsage{};
  grassDrawBufUsage._resourceName = "CullGrassDrawBuf";
  grassDrawBufUsage._access.set((std::size_t)Access::Write);
  grassDrawBufUsage._access.set((std::size_t)Access::Read);
  grassDrawBufUsage._stage.set((std::size_t)Stage::Compute);
  grassDrawBufUsage._type = Type::SSBO;
  BufferInitialCreateInfo grassDrawCreateInfo{};
  grassDrawCreateInfo._multiBuffered = true;
  grassDrawCreateInfo._initialSize = sizeof(gpu::GPUNonIndexDrawCallCmd);
  grassDrawBufUsage._bufferCreateInfo = grassDrawCreateInfo;
  regInfo._resourceUsages.emplace_back(std::move(grassDrawBufUsage));

  ResourceUsage grassObjBufUsage{};
  grassObjBufUsage._resourceName = "CullGrassObjBuf";
  grassObjBufUsage._access.set((std::size_t)Access::Write);
  grassObjBufUsage._access.set((std::size_t)Access::Read);
  grassObjBufUsage._stage.set((std::size_t)Stage::Compute);
  grassObjBufUsage._type = Type::SSBO;
  BufferInitialCreateInfo grassObjCreateInfo{};
  grassObjCreateInfo._multiBuffered = true;
  grassObjCreateInfo._initialSize = MAX_NUM_GRASS_BLADES * sizeof(gpu::GPUGrassBlade);
  grassObjBufUsage._bufferCreateInfo = grassObjCreateInfo;
  regInfo._resourceUsages.emplace_back(std::move(grassObjBufUsage));

  int currSize = 64;
  for (int i = 0; i < 6; ++i) {
    ResourceUsage depthUsage{};
    depthUsage._resourceName = "HiZ" + std::to_string(currSize);
    depthUsage._access.set((std::size_t)Access::Read);
    depthUsage._stage.set((std::size_t)Stage::Compute);
    depthUsage._multiBuffered = true; // TODO: This is not true, it isn't multi buffered.. but we have to specify since all the other buffers are
    depthUsage._useMaxSampler = true;
    depthUsage._type = Type::SampledTexture;
    regInfo._resourceUsages.emplace_back(std::move(depthUsage));
    currSize = currSize >> 1;
  }

  {
    ResourceUsage usage{};
    usage._resourceName = "CullBSDrawBuf";
    usage._access.set((std::size_t)Access::Write);
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SSBO;
    BufferInitialCreateInfo createInfo{};
    createInfo._multiBuffered = true;
    createInfo._initialSize = sizeof(gpu::GPUDrawCallCmd);
    usage._bufferCreateInfo = createInfo;
    regInfo._resourceUsages.emplace_back(std::move(usage));
  }

  {
    ResourceUsage usage{};
    usage._resourceName = "CullBSTransBuf";
    usage._access.set((std::size_t)Access::Write);
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SSBO;
    BufferInitialCreateInfo createInfo{};
    createInfo._multiBuffered = true;
    createInfo._initialSize = rc->getMaxNumRenderables() * sizeof(gpu::GPUTranslationId);
    usage._bufferCreateInfo = createInfo;
    regInfo._resourceUsages.emplace_back(std::move(usage));
  }


  ComputePipelineCreateParams pipeParam{};
  pipeParam.device = rc->device();
  //pipeParam.pipelineLayout = _pipelineLayout;
  pipeParam.shader = "cull_comp.spv";

  regInfo._computeParams = pipeParam;

  fgb.registerRenderPass(std::move(regInfo));

  fgb.registerRenderPassExe("Cull",
    [this](RenderExeParams exeParams)
    {
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

      // 32 is the local group size in the comp shader
      const uint32_t localSize = 32;
      int sqr = std::ceil(std::sqrt((double)exeParams.rc->getCurrentNumRenderables() / (double)(localSize * localSize)));
      vkCmdDispatch(*exeParams.cmdBuffer, sqr, sqr, 1);
    });
}

}