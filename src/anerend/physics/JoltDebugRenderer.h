#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRenderer.h>

#include "../render/RenderContext.h"
#include "../util/Uuid.h"

namespace physics {

class JoltDebugRenderer : public JPH::DebugRenderer
{
public:
  JoltDebugRenderer(render::RenderContext* rc);
  ~JoltDebugRenderer() = default;

  JoltDebugRenderer(const JoltDebugRenderer&) = delete;
  JoltDebugRenderer& operator=(const JoltDebugRenderer&) = delete;
  JoltDebugRenderer(JoltDebugRenderer&&) = delete;
  JoltDebugRenderer& operator=(JoltDebugRenderer&&) = delete;

  void init();

  virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override final;
  virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow = ECastShadow::Off) override final;
  virtual void DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& inGeometry, ECullMode inCullMode, ECastShadow inCastShadow, EDrawMode inDrawMode) override final;
  virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override final;

  virtual Batch CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) override final;
  virtual Batch	CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const std::uint32_t* inIndices, int inIndexCount) override final;

private:
  class BatchImpl : public JPH::RefTargetVirtual
  {
  public:
    BatchImpl(util::Uuid modelId, util::Uuid meshId, render::RenderContext* rc)
      : _modelId(modelId)
      , _meshId(meshId)
      , _rc(rc)
    {}

    virtual void AddRef() override { _refCounter++; }
    virtual void Release() override { 
      if (--_refCounter == 0) {
        render::AssetUpdate upd{};
        upd._removedModels.emplace_back(_modelId);
        _rc->assetUpdate(std::move(upd));
        delete this;
      }
    }

    util::Uuid _modelId;
    util::Uuid _meshId;
    render::RenderContext* _rc = nullptr;
    unsigned _refCounter = 0;
  };

  render::RenderContext* _rc = nullptr;
};

}