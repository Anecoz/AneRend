#include "PresentationRenderPass.h"

#include "../FrameGraphBuilder.h"
#include "../ImageHelpers.h"
#include "../RenderResource.h"
#include "../RenderResourceVault.h"

namespace render {

PresentationRenderPass::PresentationRenderPass()
  : RenderPass()
{
}

PresentationRenderPass::~PresentationRenderPass()
{
}

bool PresentationRenderPass::init(RenderContext* renderContext, RenderResourceVault* vault)
{
  return true;
}

void PresentationRenderPass::registerToGraph(FrameGraphBuilder& fgb)
{
  RenderPassRegisterInfo info{};
  info._name = "Present";
  info._present = true;

  std::vector<ResourceUsage> resourceUsage;
  {
    ResourceUsage usage{};
    usage._resourceName = "FinalImagePP";
    usage._access.set((std::size_t)Access::Read);
    usage._type = Type::Present;
    resourceUsage.emplace_back(std::move(usage));
  }

  info._resourceUsages = std::move(resourceUsage);
  fgb.registerRenderPass(std::move(info));
  fgb.registerRenderPassExe("Present",
    [](RenderResourceVault* vault, RenderContext* renderContext, VkCommandBuffer* cmdBuffer, int multiBufferIdx, int extraSz, void* extra) {

      // Copy to the present image from the swap chain
      auto geomIm = (ImageRenderResource*)vault->getResource("FinalImagePP");
      auto presentImage = renderContext->getCurrentSwapImage();

      VkImageCopy imageCopyRegion{};
      imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageCopyRegion.srcSubresource.layerCount = 1;
      imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageCopyRegion.dstSubresource.layerCount = 1;
      imageCopyRegion.extent.width = renderContext->swapChainExtent().width;
      imageCopyRegion.extent.height = renderContext->swapChainExtent().height;
      imageCopyRegion.extent.depth = 1;

      VkImageBlit blit{};
      blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.srcSubresource.layerCount = 1;
      blit.srcOffsets[1].x = renderContext->swapChainExtent().width;
      blit.srcOffsets[1].y = renderContext->swapChainExtent().height;
      blit.srcOffsets[1].z = 1;
      blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.dstSubresource.layerCount = 1;
      blit.dstOffsets[1].x = renderContext->swapChainExtent().width;
      blit.dstOffsets[1].y = renderContext->swapChainExtent().height;
      blit.dstOffsets[1].z = 1;

      vkCmdBlitImage(
        *cmdBuffer,
        geomIm->_image._image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        presentImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &blit,
        VK_FILTER_NEAREST);

      /*vkCmdCopyImage(
        *cmdBuffer,
        geomIm->_image._image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        presentImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &imageCopyRegion);*/
    });
}

}