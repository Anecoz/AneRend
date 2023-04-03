#pragma once

#include "RenderPass.h"
#include "../AllocatedBuffer.h"

namespace render {

class CullRenderPass : public RenderPass
{
public:
  CullRenderPass();
  ~CullRenderPass();

  bool init(RenderContext* renderContext, RenderResourceVault*) override final;

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&) override final;

  void cleanup(RenderContext* renderContext, RenderResourceVault* vault) override final;

private:
  // Temp
  const std::size_t MAX_NUM_GRASS_BLADES = 32 * 32 * 1000;

  std::size_t _currentStagingOffset = 0;

  std::vector<AllocatedBuffer> _gpuStagingBuffers;
  std::vector<VkDescriptorSet> _descriptorSets;
};

}