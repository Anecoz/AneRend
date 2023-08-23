#include "IrradianceProbeRayTracingPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"
#include "../BufferHelpers.h"
#include "../VulkanExtensions.h"

#include <random>

namespace render {

namespace {

struct PushConstant {
  glm::mat4 rot;
  uint32_t layer;
};

void fillProbeSSBO(RenderContext* rc, AllocatedBuffer& ssbo)
{
  auto& probes = rc->getIrradianceProbes();

  // Create a staging buffer on CPU side first
  auto allocator = rc->vmaAllocator();
  AllocatedBuffer stagingBuffer;
  std::size_t dataSize = probes.size() * sizeof(gpu::GPUIrradianceProbe);

  bufferutil::createBuffer(allocator, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer);

  gpu::GPUIrradianceProbe* mappedData;
  vmaMapMemory(allocator, stagingBuffer._allocation, &(void*)mappedData);

  const std::size_t width = rc->getNumIrradianceProbesXZ();
  const std::size_t height = rc->getNumIrradianceProbesY();
  const std::size_t depth = rc->getNumIrradianceProbesXZ();

  for (std::size_t i = 0; i < probes.size(); ++i) {
    auto& cpuProbe = probes[i];
    gpu::GPUIrradianceProbe* probe = &mappedData[i];

    *probe = cpuProbe;
  }

  vmaUnmapMemory(allocator, stagingBuffer._allocation);

  auto cmdBuffer = rc->beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.size = dataSize;
  vkCmdCopyBuffer(cmdBuffer, stagingBuffer._buffer, ssbo._buffer, 1, &copyRegion);

  rc->endSingleTimeCommands(cmdBuffer);
  vmaDestroyBuffer(allocator, stagingBuffer._buffer, stagingBuffer._allocation);
}

}

IrradianceProbeRayTracingPass::IrradianceProbeRayTracingPass()
  : RenderPass()
{}

IrradianceProbeRayTracingPass::~IrradianceProbeRayTracingPass()
{}

void IrradianceProbeRayTracingPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "IrradianceProbeRT";
  info._group = "IrradianceProbe";

  const int numRaysPerProbe = 256;
  const int sqrtNumRays = std::sqrt(numRaysPerProbe);
  const int numProbesPlane = rc->getNumIrradianceProbesXZ();
  const int numProbesHeight = rc->getNumIrradianceProbesY();

  // Texture to capture the rays irradiance information, which will later be integrated per probe oct direction
  {
    ResourceUsage usage{};
    usage._resourceName = "ProbeRaysIrrTex";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::RayTrace);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialWidth = sqrtNumRays * numProbesPlane;
    createInfo._initialHeight = sqrtNumRays * numProbesPlane;// * numProbesHeight;
    createInfo._intialFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  // Captures the direction of each ray, corresponding to the irradiance
  {
    ResourceUsage usage{};
    usage._resourceName = "ProbeRaysDirTex";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::RayTrace);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialWidth = sqrtNumRays * numProbesPlane;
    createInfo._initialHeight = sqrtNumRays * numProbesPlane;// *numProbesHeight;
    createInfo._intialFormat = VK_FORMAT_R8G8B8A8_UNORM;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageStorage;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "ProbeRaysConvTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::RayTrace);
    usage._type = Type::SampledTexture;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  /* {
    ResourceUsage usage{};
    usage._resourceName = "IrradianceProbeSSBO";
    usage._stage.set((std::size_t)Stage::Transfer);
    usage._access.set((std::size_t)Access::Read);
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::RayTrace);
    usage._type = Type::SSBO;

    BufferInitialCreateInfo createInfo{};
    createInfo._initialSize = rc->getNumIrradianceProbesXZ() * rc->getNumIrradianceProbesXZ() * rc->getNumIrradianceProbesY() * sizeof(gpu::GPUIrradianceProbe); sizeof(gpu::GPUIrradianceProbe);
    createInfo._initialDataCb = fillProbeSSBO;
    usage._bufferCreateInfo = createInfo;

    info._resourceUsages.emplace_back(std::move(usage));
  }*/

  RayTracingPipelineCreateParams param{};
  param.device = rc->device();
  param.rc = rc;
  param.raygenShader = "irradiance_probe_update_rgen.spv";
  param.missShader = "irradiance_probe_update_rmiss.spv";
  param.closestHitShader = "irradiance_probe_update_rchit.spv";
  param.maxRecursionDepth = 2;
  info._rtParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("IrradianceProbeRT",
    [this, numProbesHeight, numProbesPlane, sqrtNumRays](RenderExeParams exeParams) {
      if (!exeParams.rc->getRenderOptions().raytracingEnabled) return;
      if (!exeParams.rc->getRenderOptions().ddgiEnabled) return;

      // Bind pipeline
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, *exeParams.pipeline);

      // Descriptor set in set 1
      vkCmdBindDescriptorSets(
        *exeParams.cmdBuffer,
        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        *exeParams.pipelineLayout,
        1, 1, &(*exeParams.descriptorSets)[0],
        0, nullptr);

      // Choose a layer
      uint32_t probeLayer = _currentLayer;

      // Random orientation matrix
      std::random_device dev;
      std::mt19937 rng(dev());
      std::uniform_real_distribution<> angle(0.0, 360.0);
      std::uniform_int_distribution<> axis(0, 2);

      int ax = axis(rng);
      glm::vec3 rotAx{0.0f};

      if (ax == 0) {
        rotAx = glm::vec3(1.0f, 0.0f, 0.0f);
      }
      else if (ax == 1) {
        rotAx = glm::vec3(0.0f, 1.0f, 0.0f);
      }
      else {
        rotAx = glm::vec3(0.0f, 0.0f, 1.0f);
      }

      glm::mat4 randomRot = glm::rotate(glm::mat4(1.0f), glm::radians(float(angle(rng))), rotAx);

      PushConstant pc{ randomRot, probeLayer };

      vkCmdPushConstants(
        *exeParams.cmdBuffer,
        *exeParams.pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        0,
        sizeof(PushConstant),
        &pc);

      VkStridedDeviceAddressRegionKHR emptyRegion{};

      // Trace some rays
      vkext::vkCmdTraceRaysKHR(
        *exeParams.cmdBuffer,
        &exeParams.sbt->_rgenRegion,
        &exeParams.sbt->_missRegion,
        &exeParams.sbt->_chitRegion,
        &emptyRegion,
        sqrtNumRays * numProbesPlane,
        sqrtNumRays * numProbesPlane,
        1);

      exeParams.rc->setBlackboardValueInt("ProbeLayer", (int)_currentLayer);

      _currentLayer++;

      if (_currentLayer >= numProbesHeight) {
        _currentLayer = 0;
      }
    });
}

}