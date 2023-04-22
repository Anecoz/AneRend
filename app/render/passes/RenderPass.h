#pragma once

#include "../RenderContext.h"

//#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <optional>
#include <vector>

namespace render {

  class FrameGraphBuilder;
  class RenderResourceVault;

  /*
  Renderpass base class. Helps with setting up pipeline and things related to the renderpass.
  Also forces an interface to the FrameGraphBuilder.
  */
  class RenderPass
  {
  public:
    RenderPass() = default;
    virtual ~RenderPass() = default;

    // No move or copy
    RenderPass(const RenderPass&) = delete;
    RenderPass(RenderPass&&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass& operator=(RenderPass&&) = delete;

    // Meant to create pipeline etc as well as add resources to the vault
    // It is also possible to get already existing resources from the vault to setup descriptor sets
    virtual bool init(RenderContext* renderContext, RenderResourceVault*) { return true; };

    // Register how the render pass will actually render
    virtual void registerToGraph(FrameGraphBuilder&, RenderContext* rc) = 0;

    // Clean up any created resources
    virtual void cleanup(RenderContext* renderContext, RenderResourceVault*) {}
  };
}