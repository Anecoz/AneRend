#pragma once

#include "RenderPass.h"
#include "../../util/Uuid.h"

namespace render {

class FXAARenderPass : public RenderPass
{
public:
  FXAARenderPass();
  ~FXAARenderPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;

private:
  util::Uuid _meshId;
};

}