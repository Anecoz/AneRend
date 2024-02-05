#include "DebugDrawRenderPass.h"

#include "../FrameGraphBuilder.h"
#include "../RenderContext.h"
#include "../BufferHelpers.h"

namespace render {

DebugDrawRenderPass::DebugDrawRenderPass()
  : RenderPass()
{}

DebugDrawRenderPass::~DebugDrawRenderPass()
{}

void DebugDrawRenderPass::cleanup(RenderContext * renderContext, RenderResourceVault*)
{
  for (auto& buf : _lineBuffers) {
    vmaDestroyBuffer(renderContext->vmaAllocator(), buf._buffer, buf._allocation);
  }
  for (auto& buf : _triangleBuffers) {
    vmaDestroyBuffer(renderContext->vmaAllocator(), buf._buffer, buf._allocation);
  }
}

void DebugDrawRenderPass::registerToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  createLineBuffer(rc);
  createTriangleBuffers(rc);

  registerLineDrawToGraph(fgb, rc);
  registerTriangleDrawToGraph(fgb, rc);

  // Register both with and without wireframe
  registerGeometryDrawToGraph(fgb, rc, true);
  registerGeometryDrawToGraph(fgb, rc, false);
}

void DebugDrawRenderPass::registerLineDrawToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "DebugLineDraw";
  info._group = "Debug";

  {
    ResourceUsage usage{};
    usage._resourceName = "FinalImagePP";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::ColorAttachment;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "GeometryDepthImage";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::DepthAttachment;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  VkFormat format = rc->getHDRFormat();
  GraphicsPipelineCreateParams param{};
  param.device = rc->device();
  param.vertShader = "debug_lines_vert.spv";
  param.fragShader = "debug_lines_frag.spv";
  param.colorAttachmentCount = 1;
  param.colorFormats = { format };
  param.cullMode = VK_CULL_MODE_NONE;
  param.normalLoc = -1;
  //param.polygonMode = VK_POLYGON_MODE_LINE;
  param.vertexBinding = 1; // Use our own vertex buffer index
  param.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  info._graphicsParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("DebugLineDraw",
    [this](RenderExeParams exeParams) {
      // Upload lines
      auto lineVerts = exeParams.rc->takeCurrentDebugLines();

      if (lineVerts.empty()) return;

      auto numLines = lineVerts.size();
      if (!uploadLines(exeParams.rc, std::move(lineVerts), *exeParams.cmdBuffer)) return;

      // Lines are now in the line buffer, bind it and draw. Bind to index 1, which we specified in the create params.
      VkDeviceSize offsets = { 0 };
      vkCmdBindVertexBuffers(*exeParams.cmdBuffer, 1, 1, &_lineBuffers[exeParams.rc->getCurrentMultiBufferIdx()]._buffer, &offsets);

      // Dynamic rendering begin
      std::array<VkClearValue, 2> clearValues{};
      std::array<VkRenderingAttachmentInfoKHR, 1> colInfo{};
      clearValues[0].color = { {0.5f, 0.5f, 0.5f, 1.0f} };
      clearValues[1].depthStencil = { 1.0f, 0 };
      VkRenderingAttachmentInfoKHR color0AttachmentInfo{};
      color0AttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      color0AttachmentInfo.imageView = exeParams.colorAttachmentViews[0];
      color0AttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      color0AttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      color0AttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      color0AttachmentInfo.clearValue = clearValues[0];

      colInfo[0] = color0AttachmentInfo;

      VkRenderingAttachmentInfoKHR depthAttachmentInfo{};
      depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      depthAttachmentInfo.imageView = exeParams.depthAttachmentViews[0];
      depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
      depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      depthAttachmentInfo.clearValue = clearValues[1];

      VkRenderingInfoKHR renderInfo{};
      renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
      renderInfo.renderArea.extent = exeParams.rc->swapChainExtent();
      renderInfo.renderArea.offset = { 0, 0 };
      renderInfo.layerCount = 1;
      renderInfo.colorAttachmentCount = static_cast<uint32_t>(colInfo.size());
      renderInfo.pColorAttachments = colInfo.data();
      renderInfo.pDepthAttachment = &depthAttachmentInfo;

      vkCmdBeginRendering(*exeParams.cmdBuffer, &renderInfo);

      // Viewport and scissor
      VkViewport viewport{};
      viewport.x = 0.0f;
      viewport.y = 0.0f;
      viewport.width = static_cast<float>(exeParams.rc->swapChainExtent().width);
      viewport.height = static_cast<float>(exeParams.rc->swapChainExtent().height);
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;

      VkRect2D scissor{};
      scissor.offset = { 0, 0 };
      scissor.extent = exeParams.rc->swapChainExtent();

      // Bind pipeline
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *exeParams.pipeline);

      // Set dynamic viewport and scissor
      vkCmdSetViewport(*exeParams.cmdBuffer, 0, 1, &viewport);
      vkCmdSetScissor(*exeParams.cmdBuffer, 0, 1, &scissor);

      // Draw the lines
      vkCmdDraw(*exeParams.cmdBuffer, (uint32_t)numLines * 2, 1, 0, 0);

      // Stop dynamic rendering
      vkCmdEndRendering(*exeParams.cmdBuffer);
    }
  );
}

void DebugDrawRenderPass::registerTriangleDrawToGraph(FrameGraphBuilder& fgb, RenderContext* rc)
{
  RenderPassRegisterInfo info{};
  info._name = "DebugTriangleDraw";
  info._group = "Debug";

  {
    ResourceUsage usage{};
    usage._resourceName = "FinalImagePP";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::ColorAttachment;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "GeometryDepthImage";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::DepthAttachment;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  VkFormat format = rc->getHDRFormat();
  GraphicsPipelineCreateParams param{};
  param.device = rc->device();
  param.vertShader = "debug_lines_vert.spv";
  param.fragShader = "debug_lines_frag.spv";
  param.colorAttachmentCount = 1;
  param.colorFormats = { format };
  param.cullMode = VK_CULL_MODE_NONE;
  param.normalLoc = -1;
  //param.polygonMode = VK_POLYGON_MODE_LINE;
  param.vertexBinding = 1; // Use our own vertex buffer index
  info._graphicsParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe("DebugTriangleDraw",
    [this](RenderExeParams exeParams) {
      // Upload tris
      auto triVerts = exeParams.rc->takeCurrentDebugTriangles();

      if (triVerts.empty()) return;

      auto numTris = triVerts.size();
      if (!uploadTriangles(exeParams.rc, std::move(triVerts), *exeParams.cmdBuffer)) return;

      // Lines are now in the line buffer, bind it and draw. Bind to index 1, which we specified in the create params.
      VkDeviceSize offsets = { 0 };
      vkCmdBindVertexBuffers(*exeParams.cmdBuffer, 1, 1, &_lineBuffers[exeParams.rc->getCurrentMultiBufferIdx()]._buffer, &offsets);

      // Dynamic rendering begin
      std::array<VkClearValue, 2> clearValues{};
      std::array<VkRenderingAttachmentInfoKHR, 1> colInfo{};
      clearValues[0].color = { {0.5f, 0.5f, 0.5f, 1.0f} };
      clearValues[1].depthStencil = { 1.0f, 0 };
      VkRenderingAttachmentInfoKHR color0AttachmentInfo{};
      color0AttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      color0AttachmentInfo.imageView = exeParams.colorAttachmentViews[0];
      color0AttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      color0AttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      color0AttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      color0AttachmentInfo.clearValue = clearValues[0];

      colInfo[0] = color0AttachmentInfo;

      VkRenderingAttachmentInfoKHR depthAttachmentInfo{};
      depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      depthAttachmentInfo.imageView = exeParams.depthAttachmentViews[0];
      depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
      depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      depthAttachmentInfo.clearValue = clearValues[1];

      VkRenderingInfoKHR renderInfo{};
      renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
      renderInfo.renderArea.extent = exeParams.rc->swapChainExtent();
      renderInfo.renderArea.offset = { 0, 0 };
      renderInfo.layerCount = 1;
      renderInfo.colorAttachmentCount = static_cast<uint32_t>(colInfo.size());
      renderInfo.pColorAttachments = colInfo.data();
      renderInfo.pDepthAttachment = &depthAttachmentInfo;

      vkCmdBeginRendering(*exeParams.cmdBuffer, &renderInfo);

      // Viewport and scissor
      VkViewport viewport{};
      viewport.x = 0.0f;
      viewport.y = 0.0f;
      viewport.width = static_cast<float>(exeParams.rc->swapChainExtent().width);
      viewport.height = static_cast<float>(exeParams.rc->swapChainExtent().height);
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;

      VkRect2D scissor{};
      scissor.offset = { 0, 0 };
      scissor.extent = exeParams.rc->swapChainExtent();

      // Bind pipeline
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *exeParams.pipeline);

      // Set dynamic viewport and scissor
      vkCmdSetViewport(*exeParams.cmdBuffer, 0, 1, &viewport);
      vkCmdSetScissor(*exeParams.cmdBuffer, 0, 1, &scissor);

      // Draw the lines
      vkCmdDraw(*exeParams.cmdBuffer, (uint32_t)numTris * 3, 1, 0, 0);

      // Stop dynamic rendering
      vkCmdEndRendering(*exeParams.cmdBuffer);
    }
  );
}

void DebugDrawRenderPass::registerGeometryDrawToGraph(FrameGraphBuilder& fgb, RenderContext* rc, bool wireframe)
{
  std::string name = wireframe ? "DebugGeometryDrawWireframe" : "DebugGeometryDraw";
  RenderPassRegisterInfo info{};
  info._name = name;
  info._group = "Debug";

  {
    ResourceUsage usage{};
    usage._resourceName = "FinalImagePP";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::ColorAttachment;
    info._resourceUsages.emplace_back(std::move(usage));
  }
  {
    ResourceUsage usage{};
    usage._resourceName = "GeometryDepthImage";
    usage._access.set((std::size_t)Access::Write);
    usage._stage.set((std::size_t)Stage::Fragment);
    usage._type = Type::DepthAttachment;
    info._resourceUsages.emplace_back(std::move(usage));
  }

  VkFormat format = rc->getHDRFormat();
  GraphicsPipelineCreateParams param{};
  param.device = rc->device();
  param.vertShader = "debug_geometry_vert.spv";
  param.fragShader = "debug_lines_frag.spv";
  param.colorAttachmentCount = 1;
  param.colorFormats = { format };
  param.cullMode = VK_CULL_MODE_NONE;
  param.normalLoc = -1;
  param.polygonMode = wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
  info._graphicsParams = param;

  fgb.registerRenderPass(std::move(info));

  fgb.registerRenderPassExe(name,
    [this, wireframe](RenderExeParams exeParams) {

      std::vector<debug::Geometry> geoms;
      if (wireframe) {
        geoms = exeParams.rc->takeCurrentDebugGeometryWireframe();
      }
      else {
        geoms = exeParams.rc->takeCurrentDebugGeometry();
      }

      // Lines are now in the line buffer, bind it and draw. Bind to index 1, which we specified in the create params.
      VkDeviceSize offsets = { 0 };
      vkCmdBindVertexBuffers(*exeParams.cmdBuffer, 1, 1, &_lineBuffers[exeParams.rc->getCurrentMultiBufferIdx()]._buffer, &offsets);

      // Dynamic rendering begin
      std::array<VkClearValue, 2> clearValues{};
      std::array<VkRenderingAttachmentInfoKHR, 1> colInfo{};
      clearValues[0].color = { {0.5f, 0.5f, 0.5f, 1.0f} };
      clearValues[1].depthStencil = { 1.0f, 0 };
      VkRenderingAttachmentInfoKHR color0AttachmentInfo{};
      color0AttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      color0AttachmentInfo.imageView = exeParams.colorAttachmentViews[0];
      color0AttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      color0AttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      color0AttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      color0AttachmentInfo.clearValue = clearValues[0];

      colInfo[0] = color0AttachmentInfo;

      VkRenderingAttachmentInfoKHR depthAttachmentInfo{};
      depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      depthAttachmentInfo.imageView = exeParams.depthAttachmentViews[0];
      depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
      depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      depthAttachmentInfo.clearValue = clearValues[1];

      VkRenderingInfoKHR renderInfo{};
      renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
      renderInfo.renderArea.extent = exeParams.rc->swapChainExtent();
      renderInfo.renderArea.offset = { 0, 0 };
      renderInfo.layerCount = 1;
      renderInfo.colorAttachmentCount = static_cast<uint32_t>(colInfo.size());
      renderInfo.pColorAttachments = colInfo.data();
      renderInfo.pDepthAttachment = &depthAttachmentInfo;

      vkCmdBeginRendering(*exeParams.cmdBuffer, &renderInfo);

      // Viewport and scissor
      VkViewport viewport{};
      viewport.x = 0.0f;
      viewport.y = 0.0f;
      viewport.width = static_cast<float>(exeParams.rc->swapChainExtent().width);
      viewport.height = static_cast<float>(exeParams.rc->swapChainExtent().height);
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;

      VkRect2D scissor{};
      scissor.offset = { 0, 0 };
      scissor.extent = exeParams.rc->swapChainExtent();

      // Bind pipeline
      vkCmdBindPipeline(*exeParams.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *exeParams.pipeline);

      // Set dynamic viewport and scissor
      vkCmdSetViewport(*exeParams.cmdBuffer, 0, 1, &viewport);
      vkCmdSetScissor(*exeParams.cmdBuffer, 0, 1, &scissor);

      for (auto& geom : geoms) {
        struct Push
        {
          glm::mat4 model;
          glm::vec4 tint;
        } push;

        push.model = geom._modelToWorld;
        push.tint = glm::vec4(geom._tint, 1.0f);

        vkCmdPushConstants(
          *exeParams.cmdBuffer,
          *exeParams.pipelineLayout,
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
          0,
          sizeof(Push),
          &push);

        // Draw the geometry using the mesh id
        exeParams.rc->drawMeshId(exeParams.cmdBuffer, geom._meshId, 1);
      }

      // Stop dynamic rendering
      vkCmdEndRendering(*exeParams.cmdBuffer);
    }
  );
}

void DebugDrawRenderPass::createLineBuffer(RenderContext* rc)
{
  auto vmaAllocator = rc->vmaAllocator();

  _lineBuffers.resize(rc->getMultiBufferSize());

  for (auto i = 0; i < rc->getMultiBufferSize(); ++i) {
    bufferutil::createBuffer(
      vmaAllocator,
      MAX_NUM_LINES * 2 * sizeof(Vertex),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      0,
      _lineBuffers[i]);
  }

}

void DebugDrawRenderPass::createTriangleBuffers(RenderContext* rc)
{
  auto vmaAllocator = rc->vmaAllocator();

  _triangleBuffers.resize(rc->getMultiBufferSize());

  for (auto i = 0; i < rc->getMultiBufferSize(); ++i) {
    bufferutil::createBuffer(
      vmaAllocator,
      MAX_NUM_TRIANGLES * 3 * sizeof(Vertex),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      0,
      _triangleBuffers[i]);
  }
}

bool DebugDrawRenderPass::uploadLines(RenderContext* rc, std::vector<debug::Line>&& lines, VkCommandBuffer commandBuffer)
{
  auto& sb = rc->getStagingBuffer();
  auto dataSize = lines.size() * sizeof(Vertex) * 2;

  if (!sb.canFit(dataSize)) {
    return false;
  }

  uint8_t* data;
  vmaMapMemory(rc->vmaAllocator(), sb._buf._allocation, (void**)&data);

  // Offset according to current staging buffer usage
  data = data + sb._currentOffset;

  auto* vtxPtr = reinterpret_cast<render::Vertex*>(data);
  for (std::size_t i = 0; i < lines.size(); ++i) {
    vtxPtr[i * 2 + 0] = lines[i]._v0;
    vtxPtr[i * 2 + 1] = lines[i]._v1;
  }

  VkBufferCopy copyRegion{};
  copyRegion.dstOffset = 0;
  copyRegion.srcOffset = sb._currentOffset;
  copyRegion.size = dataSize;
  vkCmdCopyBuffer(commandBuffer, sb._buf._buffer, _lineBuffers[rc->getCurrentMultiBufferIdx()]._buffer, 1, &copyRegion);

  VkBufferMemoryBarrier memBarr{};
  memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarr.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
  memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.buffer = _lineBuffers[rc->getCurrentMultiBufferIdx()]._buffer;
  memBarr.offset = 0;
  memBarr.size = dataSize;

  vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
    0, 0, nullptr,
    1, &memBarr,
    0, nullptr);

  vmaUnmapMemory(rc->vmaAllocator(), sb._buf._allocation);

  // Update staging offset
  sb.advance(dataSize);
  return true;
}

bool DebugDrawRenderPass::uploadTriangles(RenderContext* rc, std::vector<debug::Triangle>&& triangles, VkCommandBuffer cmdBuf)
{
  auto& sb = rc->getStagingBuffer();
  auto dataSize = triangles.size() * sizeof(Vertex) * 3;

  if (!sb.canFit(dataSize)) {
    return false;
  }

  uint8_t* data;
  vmaMapMemory(rc->vmaAllocator(), sb._buf._allocation, (void**)&data);

  // Offset according to current staging buffer usage
  data = data + sb._currentOffset;

  auto* vtxPtr = reinterpret_cast<render::Vertex*>(data);
  for (std::size_t i = 0; i < triangles.size(); ++i) {
    vtxPtr[i * 3 + 0] = triangles[i]._v0;
    vtxPtr[i * 3 + 1] = triangles[i]._v1;
    vtxPtr[i * 3 + 2] = triangles[i]._v2;
  }

  VkBufferCopy copyRegion{};
  copyRegion.dstOffset = 0;
  copyRegion.srcOffset = sb._currentOffset;
  copyRegion.size = dataSize;
  vkCmdCopyBuffer(cmdBuf, sb._buf._buffer, _lineBuffers[rc->getCurrentMultiBufferIdx()]._buffer, 1, &copyRegion);

  VkBufferMemoryBarrier memBarr{};
  memBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  memBarr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarr.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
  memBarr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  memBarr.buffer = _lineBuffers[rc->getCurrentMultiBufferIdx()]._buffer;
  memBarr.offset = 0;
  memBarr.size = dataSize;

  vkCmdPipelineBarrier(
    cmdBuf,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
    0, 0, nullptr,
    1, &memBarr,
    0, nullptr);

  vmaUnmapMemory(rc->vmaAllocator(), sb._buf._allocation);

  // Update staging offset
  sb.advance(dataSize);
  return true;
}

}