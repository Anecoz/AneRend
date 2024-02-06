#include "JoltDebugRenderer.h"

#include "../render/debug/Line.h"
#include "../render/debug/Triangle.h"
#include "../render/debug/Geometry.h"

namespace physics {

JoltDebugRenderer::JoltDebugRenderer(render::RenderContext* rc)
  : _rc(rc)
{}

void JoltDebugRenderer::init()
{
  JPH::DebugRenderer::Initialize();
}

void JoltDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
  render::debug::Line line{};
  line._v0 = render::Vertex{};
  line._v0.pos = glm::vec3(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ());
  line._v0.color = glm::vec3(inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f);

  line._v1 = render::Vertex{};
  line._v1.pos = glm::vec3(inTo.GetX(), inTo.GetY(), inTo.GetZ());
  line._v1.color = glm::vec3(inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f);

  _rc->debugDrawLine(std::move(line));
}

void JoltDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow)
{
  // Ignore castshadow param
  render::debug::Triangle triangle{};
  triangle._v0 = render::Vertex{};
  triangle._v0.pos = glm::vec3(inV1.GetX(), inV1.GetY(), inV1.GetZ());
  triangle._v0.color = glm::vec3(inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f);

  triangle._v1 = render::Vertex{};
  triangle._v1.pos = glm::vec3(inV2.GetX(), inV2.GetY(), inV2.GetZ());
  triangle._v1.color = glm::vec3(inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f);

  triangle._v2 = render::Vertex{};
  triangle._v2.pos = glm::vec3(inV3.GetX(), inV3.GetY(), inV3.GetZ());
  triangle._v2.color = glm::vec3(inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f);

  _rc->debugDrawTriangle(std::move(triangle));
}

/// Draw some geometry
/// @param inModelMatrix is the matrix that transforms the geometry to world space.
/// @param inWorldSpaceBounds is the bounding box of the geometry after transforming it into world space.
/// @param inLODScaleSq is the squared scale of the model matrix, it is multiplied with the LOD distances in inGeometry to calculate the real LOD distance (so a number > 1 will force a higher LOD).
/// @param inModelColor is the color with which to multiply the vertex colors in inGeometry.
/// @param inGeometry The geometry to draw.
/// @param inCullMode determines which polygons are culled.
/// @param inCastShadow determines if this geometry should cast a shadow or not.
/// @param inDrawMode determines if we draw the geometry solid or in wireframe.
void JoltDebugRenderer::DrawGeometry(
  JPH::RMat44Arg inModelMatrix, 
  const JPH::AABox& inWorldSpaceBounds, 
  float inLODScaleSq, 
  JPH::ColorArg inModelColor, 
  const GeometryRef& inGeometry, 
  ECullMode inCullMode, 
  ECastShadow inCastShadow, 
  EDrawMode inDrawMode)
{
  auto* batch = static_cast<BatchImpl*>(inGeometry->mLODs.front().mTriangleBatch.GetPtr());
  
  render::debug::Geometry geom{};
  geom._meshId = batch->_meshId;
  geom._tint = glm::vec3(inModelColor.r / 255.0f, inModelColor.g / 255.0f, inModelColor.b / 255.0f);
  geom._modelToWorld[0] = glm::vec4(inModelMatrix.GetColumn4(0).GetX(), inModelMatrix.GetColumn4(0).GetY(), inModelMatrix.GetColumn4(0).GetZ(), inModelMatrix.GetColumn4(0).GetW());
  geom._modelToWorld[1] = glm::vec4(inModelMatrix.GetColumn4(1).GetX(), inModelMatrix.GetColumn4(1).GetY(), inModelMatrix.GetColumn4(1).GetZ(), inModelMatrix.GetColumn4(1).GetW());
  geom._modelToWorld[2] = glm::vec4(inModelMatrix.GetColumn4(2).GetX(), inModelMatrix.GetColumn4(2).GetY(), inModelMatrix.GetColumn4(2).GetZ(), inModelMatrix.GetColumn4(2).GetW());
  geom._modelToWorld[3] = glm::vec4(inModelMatrix.GetColumn4(3).GetX(), inModelMatrix.GetColumn4(3).GetY(), inModelMatrix.GetColumn4(3).GetZ(), inModelMatrix.GetColumn4(3).GetW());
  geom._wireframe = inDrawMode == EDrawMode::Wireframe;

  _rc->debugDrawGeometry(std::move(geom));
}

void JoltDebugRenderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight)
{
  // Not implemented Sadge
  std::string str(inString);
  printf("Jolt is trying to render text: %s\n", str.c_str());
}

JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount)
{
  // Create a mesh and model from the input and add to renderer (bypass scene)
  render::asset::Model model{};
  render::asset::Mesh mesh{};

  mesh._vertices.reserve(inTriangleCount * 3);
  for (int i = 0; i < inTriangleCount; ++i) {
    for (int j = 0; j < 3; ++j) {
      render::Vertex v{};
      v.pos = glm::vec3(inTriangles[i].mV[j].mPosition.x, inTriangles[i].mV[j].mPosition.y, inTriangles[i].mV[j].mPosition.z);
      v.color = glm::vec3(inTriangles[i].mV[j].mColor.r / 255.0f, inTriangles[i].mV[j].mColor.g / 255.0f, inTriangles[i].mV[j].mColor.b / 255.0f);
      v.normal = glm::vec3(inTriangles[i].mV[j].mNormal.x, inTriangles[i].mV[j].mNormal.y, inTriangles[i].mV[j].mNormal.z);
      v.uv = glm::vec2(inTriangles[i].mV[j].mUV.x, inTriangles[i].mV[j].mUV.y);

      mesh._vertices.emplace_back(std::move(v));
    }
  }

  model._name = "PhysicsModelNoIndices";
  auto meshId = mesh._id;
  model._meshes.emplace_back(std::move(mesh));

  auto batch = new BatchImpl(model._id, meshId, _rc);

  render::AssetUpdate upd{};
  upd._addedModels.emplace_back(std::move(model));
  _rc->assetUpdate(std::move(upd));

  return batch;
}

JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const std::uint32_t* inIndices, int inIndexCount)
{
  // Create a mesh and model from the input and add to renderer (bypass scene)
  render::asset::Model model{};
  render::asset::Mesh mesh{};

  mesh._vertices.reserve(inVertexCount);
  mesh._indices.resize(inIndexCount);
  for (int i = 0; i < inVertexCount; ++i) {
    render::Vertex v{};
    v.pos = glm::vec3(inVertices[i].mPosition.x, inVertices[i].mPosition.y, inVertices[i].mPosition.z);
    v.color = glm::vec3(inVertices[i].mColor.r / 255.0f, inVertices[i].mColor.g / 255.0f, inVertices[i].mColor.b / 255.0f);
    v.normal = glm::vec3(inVertices[i].mNormal.x, inVertices[i].mNormal.y, inVertices[i].mNormal.z);
    v.uv = glm::vec2(inVertices[i].mUV.x, inVertices[i].mUV.y);

    mesh._vertices.emplace_back(std::move(v));
  }

  for (int i = 0; i < inIndexCount; ++i) {
    mesh._indices[i] = inIndices[i];
  }

  model._name = "PhysicsModelWithIndices";
  auto meshId = mesh._id;
  model._meshes.emplace_back(std::move(mesh));

  auto batch = new BatchImpl(model._id, meshId, _rc);

  render::AssetUpdate upd{};
  upd._addedModels.emplace_back(std::move(model));
  _rc->assetUpdate(std::move(upd));

  return batch;
}

}