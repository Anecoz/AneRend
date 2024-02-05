#pragma once

#include "RenderPass.h"
#include "../AllocatedBuffer.h"

namespace render {

class DebugDrawRenderPass : public RenderPass
{
public:
  DebugDrawRenderPass();
  ~DebugDrawRenderPass();

  void cleanup(RenderContext* renderContext, RenderResourceVault*) override final;

  // Register how the render pass will actually render
  void registerToGraph(FrameGraphBuilder&, RenderContext* rc) override final;
private:
  void registerLineDrawToGraph(FrameGraphBuilder&, RenderContext* rc);
  void registerTriangleDrawToGraph(FrameGraphBuilder&, RenderContext* rc);
  void registerGeometryDrawToGraph(FrameGraphBuilder&, RenderContext* rc, bool wireframe);

  void createLineBuffer(RenderContext* rc);
  void createTriangleBuffers(RenderContext* rc);

  bool uploadLines(RenderContext* rc, std::vector<debug::Line>&& lines, VkCommandBuffer cmdBuf);
  bool uploadTriangles(RenderContext* rc, std::vector<debug::Triangle>&& triangles, VkCommandBuffer cmdBuf);

  const std::size_t MAX_NUM_LINES = 1000;
  const std::size_t MAX_NUM_TRIANGLES = 1000;

  std::vector<AllocatedBuffer> _lineBuffers;
  std::vector<AllocatedBuffer> _triangleBuffers;
};

}