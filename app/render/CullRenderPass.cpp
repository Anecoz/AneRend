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
  /* We need to create :
  * - the compute pipeline
  * - a DrawBuffer that will be used as an SSBO
  * - a descriptor set for the DrawBuffer
  * 
  * The rest will be coming from the bindless descriptors (renderable buffer and translation buffer)
  */

  // Desc layout for draw and translation buffer
  DescriptorBindInfo drawBufferInfo{};
  drawBufferInfo.binding = 0;
  drawBufferInfo.stages = VK_SHADER_STAGE_COMPUTE_BIT;
  drawBufferInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

  DescriptorBindInfo transBufferInfo{};
  transBufferInfo.binding = 1;
  transBufferInfo.stages = VK_SHADER_STAGE_COMPUTE_BIT;
  transBufferInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

  DescriptorSetLayoutCreateParams descLayoutParam{};
  descLayoutParam.bindInfos.emplace_back(drawBufferInfo);
  descLayoutParam.bindInfos.emplace_back(transBufferInfo);
  descLayoutParam.device = renderContext->device();

  buildDescriptorSetLayout(descLayoutParam);

  // Pipeline layout
  PipelineLayoutCreateParams pipeLayoutParam{};
  pipeLayoutParam.device = renderContext->device();
  pipeLayoutParam.descriptorSetLayouts.emplace_back(renderContext->bindlessDescriptorSetLayout()); // set 0
  pipeLayoutParam.descriptorSetLayouts.emplace_back(_descriptorSetLayout); // set 1
  pipeLayoutParam.pushConstantsSize = 256;// (uint32_t)sizeof(gpu::GPUCullPushConstants);
  pipeLayoutParam.pushConstantStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

  buildPipelineLayout(pipeLayoutParam);

  // Descriptor (multi buffered) containing draw buffer SSBO
  DescriptorSetsCreateParams descParam{};
  descParam.device = renderContext->device();
  descParam.descriptorPool = renderContext->descriptorPool();
  descParam.descriptorSetLayout = _descriptorSetLayout;
  descParam.numMultiBuffer = renderContext->getMultiBufferSize();

  // Create the buffers (multi buffered)
  _gpuStagingBuffers.resize(renderContext->getMultiBufferSize());
  auto allocator = renderContext->vmaAllocator();
  for (int i = 0; i < renderContext->getMultiBufferSize(); ++i) {
    auto resourceDrawBuf = new BufferRenderResource();
    auto resourceTransBuf = new BufferRenderResource();

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

    drawBufferInfo.buffer = resourceDrawBuf->_buffer._buffer;
    transBufferInfo.buffer = resourceTransBuf->_buffer._buffer;

    descParam.bindInfos.emplace_back(drawBufferInfo);
    descParam.bindInfos.emplace_back(transBufferInfo);

    vault->addResource("CullDrawBuf", std::unique_ptr<IRenderResource>(resourceDrawBuf), true, i);
    vault->addResource("CullTransBuf", std::unique_ptr<IRenderResource>(resourceTransBuf), true, i);

    // Create staging buffers while we're looping
    bufferutil::createBuffer(
      allocator,
      renderContext->getMaxNumRenderables() * sizeof(gpu::GPURenderable), // We use this as the size, as the GPU buffers can never get bigger than this
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

      gpu::GPUDrawCallCmd* mappedData;
      vmaMapMemory(renderContext->vmaAllocator(), _gpuStagingBuffers[renderContext->getCurrentMultiBufferIdx()]._allocation, (void**)&mappedData);
      for (std::size_t i = 0; i < meshes.size(); ++i) {
        auto& mesh = meshes[i];
        mappedData[i]._command.indexCount = (uint32_t)mesh._numIndices;
        mappedData[i]._command.instanceCount = 0; // Updated by GPU compute shader
        mappedData[i]._command.firstIndex = (uint32_t)mesh._indexOffset;
        mappedData[i]._command.vertexOffset = (uint32_t)mesh._vertexOffset;
        mappedData[i]._command.firstInstance = i == 0 ? 0 : (uint32_t)renderContext->getCurrentMeshUsage()[(uint32_t)i - 1];
      }

      VkBufferCopy copyRegion{};
      copyRegion.dstOffset = 0;
      copyRegion.size = dataSize;
      vkCmdCopyBuffer(cmdBuffer, _gpuStagingBuffers[renderContext->getCurrentMultiBufferIdx()]._buffer, buf->_buffer._buffer, 1, &copyRegion);

      vmaUnmapMemory(renderContext->vmaAllocator(), _gpuStagingBuffers[renderContext->getCurrentMultiBufferIdx()]._allocation);
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

  // Register the actual render pass
  RenderPassRegisterInfo regInfo{};
  regInfo._name = "Cull";
  
  ResourceUsage drawCmdUsage{};
  drawCmdUsage._resourceName = "CullDrawBuf";
  drawCmdUsage._access.set((std::size_t)Access::Write);
  drawCmdUsage._stage.set((std::size_t)Stage::Compute);
  drawCmdUsage._type = Type::SSBO;
  regInfo._resourceUsages.emplace_back(std::move(drawCmdUsage));

  ResourceUsage translationBufUsage{};
  translationBufUsage._resourceName = "CullTransBuf";
  translationBufUsage._access.set((std::size_t)Access::Write);
  translationBufUsage._stage.set((std::size_t)Stage::Compute);
  translationBufUsage._type = Type::SSBO;
  regInfo._resourceUsages.emplace_back(std::move(translationBufUsage));

  fgb.registerRenderPass(std::move(regInfo));

  fgb.registerRenderPassExe("Cull",
    [this](RenderResourceVault* vault, RenderContext* renderContext, VkCommandBuffer* cmdBuffer, int multiBufferIdx)
    {
      vkCmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);

      // Bind to set 1
      vkCmdBindDescriptorSets(
        *cmdBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        _pipelineLayout,
        1, 1, &_descriptorSets[multiBufferIdx],
        0, nullptr);

      auto pushConstants = renderContext->getCullParams();
      vkCmdPushConstants(*cmdBuffer, _pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(gpu::GPUCullPushConstants), &pushConstants);

      // 256 is the local group size in the comp shader
      uint32_t numGroups = static_cast<uint32_t>(((uint32_t)renderContext->getCurrentNumRenderables() / 256) + 1);
      vkCmdDispatch(*cmdBuffer, numGroups, 1, 1);
    });
}

void CullRenderPass::cleanup(RenderContext* renderContext, RenderResourceVault* vault)
{
  for (int i = 0; i < renderContext->getMultiBufferSize(); ++i) {
    auto drawBuf = (BufferRenderResource*)vault->getResource("CullDrawBuf", i);
    vmaDestroyBuffer(renderContext->vmaAllocator(), drawBuf->_buffer._buffer, drawBuf->_buffer._allocation);

    auto transBuf = (BufferRenderResource*)vault->getResource("CullTransBuf", i);
    vmaDestroyBuffer(renderContext->vmaAllocator(), transBuf->_buffer._buffer, transBuf->_buffer._allocation);

    vmaDestroyBuffer(renderContext->vmaAllocator(), _gpuStagingBuffers[i]._buffer, _gpuStagingBuffers[i]._allocation);
  }

  vkDestroyDescriptorSetLayout(renderContext->device(), _descriptorSetLayout, nullptr);
  vkDestroyPipelineLayout(renderContext->device(), _pipelineLayout, nullptr);
  vkDestroyPipeline(renderContext->device(), _pipeline, nullptr);

  vault->deleteResource("CullDrawBuf");
  vault->deleteResource("CullTransBuf");
}

}