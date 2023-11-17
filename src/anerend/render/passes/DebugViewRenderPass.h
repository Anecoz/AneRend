#pragma once

#include "RenderPass.h"

#include "../FrameGraphBuilder.h"

#include "../asset/Model.h"

namespace render {

class DebugViewRenderPass : public RenderPass
{
public:
  DebugViewRenderPass();
  ~DebugViewRenderPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;

private:
  uint32_t translateNameToBinding(const std::string& name);

  std::vector<ResourceUsage> _resourceUsages;

  util::Uuid _meshId;

  std::string _currResource;
};

}