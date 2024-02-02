#pragma once

#include "RenderPass.h"

namespace render {

class GrassGenPass : public RenderPass
{
public:
  GrassGenPass();
  ~GrassGenPass();

  void cleanup(RenderContext* renderContext, RenderResourceVault*) override final;

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
private:
  const std::size_t MAX_NUM_GRASS_BLADES = 32 * 32 * 1000;

  //std::size_t _currentStagingOffset = 0;

  std::vector<AllocatedBuffer> _gpuStagingBuffers;
};

}