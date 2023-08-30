#pragma once

#include "RenderPass.h"

namespace render {

class UpdateTLASPass : public RenderPass
{
public:
  UpdateTLASPass();
  ~UpdateTLASPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
};

}