#include "HiZRenderPass.h"

#include "../FrameGraphBuilder.h"
#include "../GpuBuffers.h"

namespace render
{

HiZRenderPass::HiZRenderPass()
  : RenderPass()
{}

HiZRenderPass::~HiZRenderPass()
{}

void HiZRenderPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  // Takes the (previous frame's) depth buffer and downsamples into
  // a couple of low-res versions, i.e. a manual mipmap.
  // Sizes:
  // 64
  // 32
  // 16
  // 8
  // 4
  // 2

  RenderPassRegisterInfo info{};
  info._name = "HiZ";

  // Register all mips as both sampled depth texture and imagestore, 
  // because the compute pass later runs in a successive fashion where 
  // the previous mip is used as input (sampler) for the current output mip (imagestore)
  {
    ResourceUsage usage{};
    usage._resourceName = "GeometryDepthImage";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Compute);
    usage._type = Type::SampledDepthTexture;
    usage._useMaxSampler = true;

    info._resourceUsages.emplace_back(std::move(usage));
  }

  int currSize = 64;
  for (int i = 0; i < 6; ++i) {
    {
      ResourceUsage usage{};
      usage._resourceName = "HiZ" + std::to_string(currSize);
      usage._access.set((std::size_t)Access::Read);
      usage._stage.set((std::size_t)Stage::Compute);
      usage._type = Type::SampledTexture;
      usage._useMaxSampler = true;
      usage._imageAlwaysGeneral = true;

      ImageInitialCreateInfo createInfo{};
      createInfo._initialHeight = currSize;
      createInfo._initialWidth = currSize;
      createInfo._intialFormat = VK_FORMAT_R32_SFLOAT;
      usage._imageCreateInfo = createInfo;

      info._resourceUsages.emplace_back(std::move(usage));
    }
    {
      ResourceUsage usage{};
      usage._resourceName = "HiZ" + std::to_string(currSize);
      usage._access.set((std::size_t)Access::Write);
      usage._stage.set((std::size_t)Stage::Compute);
      usage._type = Type::ImageStorage;

      info._resourceUsages.emplace_back(std::move(usage));
    }

    currSize = currSize >> 1;
  }

  ComputePipelineCreateParams compParams{};
  compParams.device = rc->device();
  compParams.shader = "hiz_comp.spv";

  info._computeParams = compParams;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("HiZ",
    [this](RenderExeParams exeParams) {
      // Run downsampling for each mip level

      int currSize = 128; // start one too high
      for (int i = 0; i < 6; ++i) {
        gpu::HiZPushConstants push{};

        push.inputIdx = 1 + 2 * (i - 1);
        push.outputIdx = push.inputIdx + 3;

        if (i == 0) {
          push.inputIdx = 0;
          push.outputIdx = 2;
          push.inputHeight = exeParams.rc->swapChainExtent().height;
          push.inputWidth = exeParams.rc->swapChainExtent().width;
          push.outputSize = 64;
        }
        else {
          push.inputWidth = currSize;
          push.outputSize = currSize >> 1;
        }

        vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *exeParams.pipeline);

        // Bind to set 1
        vkCmdBindDescriptorSets(
          *exeParams.cmdBuffer,
          VK_PIPELINE_BIND_POINT_COMPUTE,
          *exeParams.pipelineLayout,
          1, 1, &(*exeParams.descriptorSets)[0],
          0, nullptr);

        vkCmdPushConstants(
          *exeParams.cmdBuffer,
          *exeParams.pipelineLayout,
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
          0,
          sizeof(gpu::HiZPushConstants),
          &push);

        // 32 is the local group size in the comp shader
        const uint32_t localSize = 4;
        uint32_t num = push.outputSize / localSize;
        
        if (num == 0) {
          num = 1;
        }

        vkCmdDispatch(*exeParams.cmdBuffer, num, num, 1);

        // Manual barrier before next iteration
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = exeParams.images[push.outputIdx];
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
          *exeParams.cmdBuffer,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
          0,
          0, nullptr,
          0, nullptr,
          1, &barrier
        );

        currSize = currSize >> 1;
      }
    }
  );
}

}