#pragma once

#include "RenderPass.h"

namespace render {

class UIRenderPass : public RenderPass
{
public:
  UIRenderPass();
  ~UIRenderPass();

  bool init(RenderContext* renderContext, RenderResourceVault*) override final;

  void registerToGraph(FrameGraphBuilder&) override final;
};

}