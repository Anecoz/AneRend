#include "CullRenderPass.h"

#include "GpuBuffers.h"
#include "BufferHelpers.h"
#include "RenderResource.h"
#include "RenderResourceVault.h"
#include "FrameGraphBuilder.h"

namespace render {

CullRenderPass::CullRenderPass()
  : RenderPass()
{}

CullRenderPass::~CullRenderPass()
{
}

bool CullRenderPass::init(RenderContext* renderContext, RenderResourceVault* vault)
{
  // Desc layout for draw and translation buffer
  DescriptorBindInfo drawBufferInfo{};
  drawBufferInfo.binding = 0;
  drawBufferInfo.stages = VK_SHADER_STAGE_COMPUTE_BIT;
  drawBufferInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

  DescriptorBindInfo transBufferInfo{};
  transBufferInfo.binding = 1;
  transBufferInfo.stages = VK_SHADER_STAGE_COMPUTE_BIT;
  transBufferInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

  // Grass buffers (Draw cmds (non-indexed), translation and Grass Object buffer)
  DescriptorBindInfo grassDrawBufferInfo{};
  grassDrawBufferInfo.binding = 2;
  grassDrawBufferInfo.stages = VK_SHADER_STAGE_COMPUTE_BIT;
  grassDrawBufferInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

  DescriptorBindInfo grassObjBufferInfo{};
  grassObjBufferInfo.binding = 3;
  grassObjBufferInfo.stages = VK_SHADER_STAGE_COMPUTE_BIT;
  grassObjBufferInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

  DescriptorSetLayoutCreateParams descLayoutParam{};
  descLayoutParam.bindInfos.emplace_back(drawBufferInfo);
  descLayoutParam.bindInfos.emplace_back(transBufferInfo);
  descLayoutParam.bindInfos.emplace_back(grassDrawBufferInfo);
  descLayoutParam.bindInfos.emplace_back(grassObjBufferInfo);
  descLayoutParam.renderContext = renderContext;

  buildDescriptorSetLayout(descLayoutParam);

  // Descriptors (multi buffered) containing the SSBOs
  DescriptorSetsCreateParams descParam{};
  descParam.renderContext = renderContext;

  // Create the buffers (multi buffered)
  _gpuStagingBuffers.resize(renderContext->getMultiBufferSize());
  auto allocator = renderContext->vmaAllocator();
  for (int i = 0; i < renderContext->getMultiBufferSize(); ++i) {
    auto resourceDrawBuf = new BufferRenderResource();
    auto resourceTransBuf = new BufferRenderResource();
    auto resourceGrassDrawBuf = new BufferRenderResource();
    auto resourceGrassObjBuf = new BufferRenderResource();

    bufferutil::createBuffer(
      allocator,
      renderContext->getMaxNumMeshes() * sizeof(gpu::GPUDrawCallCmd),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0,
      resourceDrawBuf->_buffer);

    bufferutil::createBuffer(
      allocator,
      renderContext->getMaxNumRenderables() * sizeof(uint32_t),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0,
      resourceTransBuf->_buffer);

    // Grass draw
    bufferutil::createBuffer(
      allocator,
      sizeof(gpu::GPUNonIndexDrawCallCmd),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
      0,
      resourceGrassDrawBuf->_buffer);

    // Grass object
    bufferutil::createBuffer(
      allocator,
      MAX_NUM_GRASS_BLADES * sizeof(gpu::GPUGrassBlade),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      0,
      resourceGrassObjBuf->_buffer);

    drawBufferInfo.buffer = resourceDrawBuf->_buffer._buffer;
    transBufferInfo.buffer = resourceTransBuf->_buffer._buffer;
    grassDrawBufferInfo.buffer = resourceGrassDrawBuf->_buffer._buffer;
    grassObjBufferInfo.buffer = resourceGrassObjBuf->_buffer._buffer;

    renderContext->setDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)resourceDrawBuf->_buffer._buffer, "CullDrawBuf");
    renderContext->setDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)resourceTransBuf->_buffer._buffer, "CullTransBuf");
    renderContext->setDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)resourceGrassDrawBuf->_buffer._buffer, "CullGrassDrawBuf");
    renderContext->setDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)resourceGrassObjBuf->_buffer._buffer, "CullGrassObjBuf");

    descParam.bindInfos.emplace_back(drawBufferInfo);
    descParam.bindInfos.emplace_back(transBufferInfo);
    descParam.bindInfos.emplace_back(grassDrawBufferInfo);
    descParam.bindInfos.emplace_back(grassObjBufferInfo);

    vault->addResource("CullDrawBuf", std::unique_ptr<IRenderResource>(resourceDrawBuf), true, i);
    vault->addResource("CullTransBuf", std::unique_ptr<IRenderResource>(resourceTransBuf), true, i);
    vault->addResource("CullGrassDrawBuf", std::unique_ptr<IRenderResource>(resourceGrassDrawBuf), true, i);
    vault->addResource("CullGrassObjBuf", std::unique_ptr<IRenderResource>(resourceGrassObjBuf), true, i);

    // Create staging buffers while we're looping
    bufferutil::createBuffer(
      allocator,
      renderContext->getMaxNumRenderables() * sizeof(gpu::GPURenderable) * 2, // We use this as the size, as the GPU buffers can never get bigger than this
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
      _gpuStagingBuffers[i]);
  }
  
  _descriptorSets = buildDescriptorSets(descParam);

  // Create the compute pipeline
  ComputePipelineCreateParams pipeParam{};
  pipeParam.device = renderContext->device();
  pipeParam.pipelineLayout = _pipelineLayout;
  pipeParam.shader = "cull_comp.spv";
  buildComputePipeline(pipeParam);

  return true;
}

void CullRenderPass::registerToGraph(FrameGraphBuilder& fgb)
{
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
      for (std::size_t i = 0; i < meshes.size(); ++i) {
        auto& mesh = meshes[i];
        mappedData[i]._command.indexCount = (uint32_t)mesh._numIndices;
        mappedData[i]._command.instanceCount = 0; // Updated by GPU compute shader
        mappedData[i]._command.firstIndex = (uint32_t)mesh._indexOffset;
        mappedData[i]._command.vertexOffset = (uint32_t)mesh._vertexOffset;
        mappedData[i]._command.firstInstance = i == 0 ? 0 : (uint32_t)renderContext->getCurrentMeshUsage()[(uint32_t)i - 1];
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

  // Add an initializer for the grass obj buffer
  ResourceUsage grassObjBufInitUsage{};
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
    });

  // Register the actual render pass
  RenderPassRegisterInfo regInfo{};
  regInfo._name = "Cull";
  
  ResourceUsage drawCmdUsage{};
  drawCmdUsage._resourceName = "CullDrawBuf";
  drawCmdUsage._access.set((std::size_t)Access::Write);
  drawCmdUsage._access.set((std::size_t)Access::Read);
  drawCmdUsage._stage.set((std::size_t)Stage::Compute);
  drawCmdUsage._type = Type::SSBO;
  regInfo._resourceUsages.emplace_back(std::move(drawCmdUsage));

  ResourceUsage translationBufUsage{};
  translationBufUsage._resourceName = "CullTransBuf";
  translationBufUsage._access.set((std::size_t)Access::Write);
  translationBufUsage._access.set((std::size_t)Access::Read);
  translationBufUsage._stage.set((std::size_t)Stage::Compute);
  translationBufUsage._type = Type::SSBO;
  regInfo._resourceUsages.emplace_back(std::move(translationBufUsage));

  ResourceUsage grassDrawBufUsage{};
  grassDrawBufUsage._resourceName = "CullGrassDrawBuf";
  grassDrawBufUsage._access.set((std::size_t)Access::Write);
  grassDrawBufUsage._access.set((std::size_t)Access::Read);
  grassDrawBufUsage._stage.set((std::size_t)Stage::Compute);
  grassDrawBufUsage._type = Type::SSBO;
  regInfo._resourceUsages.emplace_back(std::move(grassDrawBufUsage));

  ResourceUsage grassObjBufUsage{};
  grassObjBufUsage._resourceName = "CullGrassObjBuf";
  grassObjBufUsage._access.set((std::size_t)Access::Write);
  grassObjBufUsage._access.set((std::size_t)Access::Read);
  grassObjBufUsage._stage.set((std::size_t)Stage::Compute);
  grassObjBufUsage._type = Type::SSBO;
  regInfo._resourceUsages.emplace_back(std::move(grassObjBufUsage));

  fgb.registerRenderPass(std::move(regInfo));

  fgb.registerRenderPassExe("Cull",
    [this](RenderResourceVault* vault, RenderContext* renderContext, VkCommandBuffer* cmdBuffer, int multiBufferIdx, int extraSz, void* extra)
    {
      if (extraSz > 0) {
        // TODO: Get culling param resource from vault via extra data
      }

      vkCmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);

      // Bind to set 1
      vkCmdBindDescriptorSets(
        *cmdBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        _pipelineLayout,
        1, 1, &_descriptorSets[multiBufferIdx],
        0, nullptr);

      auto pushConstants = renderContext->getCullParams();
      auto windStrength = renderContext->getDebugOptions().windStrength;

      uint8_t pushData[256];
      memcpy(pushData, &pushConstants, sizeof(gpu::GPUCullPushConstants));
      memcpy(pushData + sizeof(gpu::GPUCullPushConstants), &windStrength, sizeof(float));
      vkCmdPushConstants(
        *cmdBuffer,
        _pipelineLayout, 
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, 
        0,
        sizeof(gpu::GPUCullPushConstants) + sizeof(float),
        pushData);

      // 32 is the local group size in the comp shader
      const uint32_t localSize = 32;
      int sqr = 20;// std::ceil(std::sqrt((double)renderContext->getCurrentNumRenderables() / (double)(localSize * localSize)));
      vkCmdDispatch(*cmdBuffer, sqr, sqr, 1);
    });
}

void CullRenderPass::cleanup(RenderContext* renderContext, RenderResourceVault* vault)
{
  for (int i = 0; i < renderContext->getMultiBufferSize(); ++i) {
    auto drawBuf = (BufferRenderResource*)vault->getResource("CullDrawBuf", i);
    vmaDestroyBuffer(renderContext->vmaAllocator(), drawBuf->_buffer._buffer, drawBuf->_buffer._allocation);

    auto transBuf = (BufferRenderResource*)vault->getResource("CullTransBuf", i);
    vmaDestroyBuffer(renderContext->vmaAllocator(), transBuf->_buffer._buffer, transBuf->_buffer._allocation);

    auto grassDrawBuf = (BufferRenderResource*)vault->getResource("CullGrassDrawBuf", i);
    vmaDestroyBuffer(renderContext->vmaAllocator(), grassDrawBuf->_buffer._buffer, grassDrawBuf->_buffer._allocation);

    auto grassObjBuf = (BufferRenderResource*)vault->getResource("CullGrassObjBuf", i);
    vmaDestroyBuffer(renderContext->vmaAllocator(), grassObjBuf->_buffer._buffer, grassObjBuf->_buffer._allocation);

    vmaDestroyBuffer(renderContext->vmaAllocator(), _gpuStagingBuffers[i]._buffer, _gpuStagingBuffers[i]._allocation);
  }

  vkDestroyDescriptorSetLayout(renderContext->device(), _descriptorSetLayout, nullptr);
  vkDestroyPipelineLayout(renderContext->device(), _pipelineLayout, nullptr);
  vkDestroyPipeline(renderContext->device(), _pipeline, nullptr);

  vault->deleteResource("CullDrawBuf");
  vault->deleteResource("CullTransBuf");
  vault->deleteResource("CullGrassDrawBuf");
  vault->deleteResource("CullGrassObjBuf");
}

}