#pragma once

#include "RenderPass.h"

namespace render {

class IrradianceProbeTranslationPass : public RenderPass
{
public:
  IrradianceProbeTranslationPass();
  ~IrradianceProbeTranslationPass();

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;

private:
  glm::ivec2 _lastProbeIdx = {};
};

}