#include "UIRenderPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderResource.h"
#include "../RenderResourceVault.h"

#include "../../imgui/imgui.h"
#include "../../imgui/imgui_impl_vulkan.h"

namespace render {

UIRenderPass::UIRenderPass()
  : RenderPass()
{
}

UIRenderPass::~UIRenderPass()
{
}

bool UIRenderPass::init(RenderContext* renderContext, RenderResourceVault*)
{
  return true;
}

void UIRenderPass::registerToGraph(FrameGraphBuilder& fgb)
{
  RenderPassRegisterInfo info{};
  info._name = "UI";

  std::vector<ResourceUsage> resourceUsage;
  {
    ResourceUsage usage{};
    usage._resourceName = "GeometryColorImage";
    usage._access.set((std::size_t)Access::Read);
    usage._access.set((std::size_t)Access::Write);
    usage._type = Type::ColorAttachment;
    resourceUsage.emplace_back(std::move(usage));
  }

  info._resourceUsages = std::move(resourceUsage);
  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("UI",
    [](RenderResourceVault* vault, RenderContext* renderContext, VkCommandBuffer* cmdBuffer, int multiBufferIdx, int extraSz, void* extra)
    {
      // Just render the ImGui stuff on top of the frame
      auto imageView = (ImageViewRenderResource*)vault->getResource("GeometryColorImageView");

      VkClearValue clearValue{};
      clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };
      VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
      colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      colorAttachmentInfo.imageView = imageView->_view;
      colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachmentInfo.clearValue = clearValue;

      VkRenderingInfoKHR renderInfo{};
      renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
      renderInfo.renderArea.extent = renderContext->swapChainExtent();
      renderInfo.renderArea.offset = { 0, 0 };
      renderInfo.layerCount = 1;
      renderInfo.colorAttachmentCount = 1;
      renderInfo.pColorAttachments = &colorAttachmentInfo;
      renderInfo.pDepthAttachment = nullptr;

      vkCmdBeginRendering(*cmdBuffer, &renderInfo);

      ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmdBuffer);

      vkCmdEndRendering(*cmdBuffer);
    });
}

}