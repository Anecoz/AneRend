#include "PresentationRenderPass.h"

#include "../FrameGraphBuilder.h"
#include "../ImageHelpers.h"
#include "../RenderResource.h"
#include "../RenderResourceVault.h"

namespace render {

PresentationRenderPass::PresentationRenderPass()
  : RenderPass()
{}

PresentationRenderPass::~PresentationRenderPass()
{}

void PresentationRenderPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
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
    [](RenderExeParams exeParams) {

      // Copy to the present image from the swap chain
      auto presentImage = exeParams.rc->getCurrentSwapImage();

      VkImageCopy imageCopyRegion{};
      imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageCopyRegion.srcSubresource.layerCount = 1;
      imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageCopyRegion.dstSubresource.layerCount = 1;
      imageCopyRegion.extent.width = exeParams.rc->swapChainExtent().width;
      imageCopyRegion.extent.height = exeParams.rc->swapChainExtent().height;
      imageCopyRegion.extent.depth = 1;

      VkImageBlit blit{};
      blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.srcSubresource.layerCount = 1;
      blit.srcOffsets[1].x = exeParams.rc->swapChainExtent().width;
      blit.srcOffsets[1].y = exeParams.rc->swapChainExtent().height;
      blit.srcOffsets[1].z = 1;
      blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.dstSubresource.layerCount = 1;
      blit.dstOffsets[1].x = exeParams.rc->swapChainExtent().width;
      blit.dstOffsets[1].y = exeParams.rc->swapChainExtent().height;
      blit.dstOffsets[1].z = 1;

      vkCmdBlitImage(
        *exeParams.cmdBuffer,
        exeParams.presentImage,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        presentImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &blit,
        VK_FILTER_NEAREST);
    });
}

}