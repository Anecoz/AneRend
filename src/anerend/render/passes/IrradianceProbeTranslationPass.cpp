#include "IrradianceProbeTranslationPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"
#include "../ImageHelpers.h"

namespace render {

IrradianceProbeTranslationPass::IrradianceProbeTranslationPass()
  : RenderPass()
{}

IrradianceProbeTranslationPass::~IrradianceProbeTranslationPass()
{}

void IrradianceProbeTranslationPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "IrradianceProbeTrans";
  info._group = "IrradianceProbe";

  const int octPixelSize = 8;
  const int numProbesPlane = static_cast<int>(rc->getNumIrradianceProbesXZ());
  const int numProbesHeight = static_cast<int>(rc->getNumIrradianceProbesY());

  const int width = (octPixelSize + 2) * numProbesPlane; // 1 pix border around each probe
  const int height = (octPixelSize + 2) * numProbesPlane * numProbesHeight; // ditto

  {
    ResourceUsage usage{};
    usage._resourceName = "TempProbeTex";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Transfer);

    ImageInitialCreateInfo createInfo{};
    createInfo._initialWidth = width;
    createInfo._initialHeight = height;
    createInfo._intialFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

    usage._imageCreateInfo = createInfo;

    usage._type = Type::ImageTransferDst;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "ProbeRaysConvTex";
    usage._access.set((std::size_t)Access::Read);
    usage._stage.set((std::size_t)Stage::Transfer);
    usage._type = Type::ImageTransferSrc;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("IrradianceProbeTrans",
    [this, width, height, numProbesPlane, numProbesHeight, octPixelSize](RenderExeParams exeParams) {
      if (!exeParams.rc->getRenderOptions().ddgiEnabled) return;
      if (exeParams.rc->isBaking()) return;

      const glm::vec3 probeStep = { 1.0, 2.0, 1.0 };

      // Find the delta in x and z (y is irrelevant)
      auto camPos = exeParams.rc->getCamera().getPosition();
      glm::ivec2 probeIdx = { (int)(camPos.x / probeStep.x), (int)(camPos.z / probeStep.z) };
      glm::ivec2 diff = probeIdx - _lastProbeIdx;

      // This should typically at most be +-1 in either direction (unless we can move really really fast)
      int dx = (int)(diff.x);
      int dz = (int)(diff.y);

      if (dx == 0 && dz == 0) {
        // Safely return
        return;
      }

      _lastProbeIdx = probeIdx;

      auto tmpImage = exeParams.images[0];
      auto probeImage = exeParams.images[1];

      // Start by copying the probe tex into the temp tex
      VkImageCopy imageCopyRegion{};
      imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageCopyRegion.srcSubresource.layerCount = 1;
      imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageCopyRegion.dstSubresource.layerCount = 1;
      imageCopyRegion.extent.width = width;
      imageCopyRegion.extent.height = height;
      imageCopyRegion.extent.depth = 1;

      vkCmdCopyImage(
        *exeParams.cmdBuffer,
        probeImage,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        tmpImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &imageCopyRegion
      );

      // Barrier for initial copy
      imageutil::transitionImageLayout(
        *exeParams.cmdBuffer,
        tmpImage,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
      );

      imageutil::transitionImageLayout(
        *exeParams.cmdBuffer,
        probeImage,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
      );

      // Now figure out the individual regions to copy
      std::vector<VkImageCopy> imageCopies;
      int probePixSize = octPixelSize + 2;
      int planeHeight = width;
      for (int i = 0; i < numProbesHeight; ++i) {
        int planeYOffset = i * probePixSize * numProbesPlane;

        VkImageCopy imCopy{};
        imCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imCopy.srcSubresource.layerCount = 1;
        imCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imCopy.dstSubresource.layerCount = 1;
        imCopy.extent.width = width;
        imCopy.extent.height = planeHeight;
        imCopy.extent.depth = 1;

        imCopy.dstOffset.y = planeYOffset;
        imCopy.srcOffset.y = planeYOffset;

        if (dx > 0) {
          imCopy.extent.width = width - probePixSize;
          imCopy.srcOffset.x = probePixSize;
          imCopy.dstOffset.x = 0;
        }
        else if (dx < 0) {
          imCopy.extent.width = width - probePixSize;
          imCopy.srcOffset.x = 0;
          imCopy.dstOffset.x = probePixSize;
        }

        if (dz > 0) {
          imCopy.extent.height = planeHeight - probePixSize;
          imCopy.srcOffset.y += probePixSize;
          imCopy.dstOffset.y += 0;
        }
        else if (dz < 0) {
          imCopy.extent.height = planeHeight - probePixSize;
          imCopy.srcOffset.y += 0;
          imCopy.dstOffset.y += probePixSize;
        }

        imageCopies.emplace_back(std::move(imCopy));
      }

      vkCmdCopyImage(
        *exeParams.cmdBuffer,
        tmpImage,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        probeImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(imageCopies.size()), imageCopies.data());

      // Take the layouts back where the frame graph expects them
      imageutil::transitionImageLayout(
        *exeParams.cmdBuffer,
        tmpImage,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
      );

      imageutil::transitionImageLayout(
        *exeParams.cmdBuffer,
        probeImage,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
      );
    });
}

}