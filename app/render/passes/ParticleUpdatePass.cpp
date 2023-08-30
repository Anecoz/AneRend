#include "ParticleUpdatePass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"
#include "../GpuBuffers.h"
#include "../BufferHelpers.h"

namespace render {

namespace {

void prefillParticleBuffer(RenderContext* rc, AllocatedBuffer& buffer)
{
  auto particles = rc->getParticles();

  // Create a staging buffer on CPU side first
  auto allocator = rc->vmaAllocator();
  AllocatedBuffer stagingBuffer;
  std::size_t dataSize = particles.size() * sizeof(gpu::GPUParticle);

  bufferutil::createBuffer(allocator, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer);

  gpu::GPUParticle* mappedData;
  vmaMapMemory(allocator, stagingBuffer._allocation, &(void*)mappedData);

  for (std::size_t i = 0; i < particles.size(); ++i) {
    auto& cpuParticle = particles[i];
    gpu::GPUParticle* gpuParticle = &mappedData[i];
    
    gpuParticle->initialPosition = glm::vec4(cpuParticle._initialPosition, cpuParticle._lifetime);
    gpuParticle->initialVelocity = glm::vec4(cpuParticle._initialVelocity, cpuParticle._scale);
    gpuParticle->currentPosition = glm::vec4(0.0f, 0.0f, 0.0f, cpuParticle._spawnDelay);
    gpuParticle->currentVelocity = glm::vec4(0.0f);
    gpuParticle->renderableId = cpuParticle._renderableId;
    gpuParticle->alive = 0;
  }

  vmaUnmapMemory(allocator, stagingBuffer._allocation);

  auto cmdBuffer = rc->beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.size = dataSize;
  vkCmdCopyBuffer(cmdBuffer, stagingBuffer._buffer, buffer._buffer, 1, &copyRegion);

  // Barrier after filling
  VkBufferMemoryBarrier memBarr{};
  memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarr.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.buffer = buffer._buffer;
  memBarr.offset = 0;
  memBarr.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(
    cmdBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    0, 0, nullptr,
    1, &memBarr,
    0, nullptr);

  rc->endSingleTimeCommands(cmdBuffer);
  vmaDestroyBuffer(allocator, stagingBuffer._buffer, stagingBuffer._allocation);
}

}

ParticleUpdatePass::ParticleUpdatePass()
  : RenderPass()
{}

ParticleUpdatePass::~ParticleUpdatePass()
{}

void ParticleUpdatePass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "ParticleUpdate";

  {
    ResourceUsage usage{};
    usage._resourceName = "RenderableBuffer";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SSBO;
    usage._multiBuffered = true;
    usage._ownedByEngine = true;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "ParticleBuffer";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._stage.set((std::size_t)Stage::Transfer);
    usage._type = Type::SSBO;
    BufferInitialCreateInfo createInfo{};
    createInfo._initialSize = 50000 * sizeof(gpu::GPUParticle); // Hack
    createInfo._initialDataCb = prefillParticleBuffer;
    usage._bufferCreateInfo = createInfo;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  ComputePipelineCreateParams pipeParam{};
  pipeParam.device = rc->device();
  pipeParam.shader = "particle_update_comp.spv";

  info._computeParams = pipeParam;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("ParticleUpdate",
    [this](RenderExeParams exeParams) {

      // Bind pipeline
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[0],
        0, nullptr);

      uint32_t numX = 50000 / 8;

      vkCmdDispatch(*exeParams.cmdBuffer, numX + 1, 1, 1);
    });
}

}