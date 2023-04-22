#pragma once

#include "RenderPass.h"
#include "../AllocatedBuffer.h"
#include "../Model.h"

namespace render {

class SSAORenderPass : public RenderPass
{
public:
  SSAORenderPass();
  ~SSAORenderPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;

private:
  Model _screenQuad;
  MeshId _meshId;
};

}