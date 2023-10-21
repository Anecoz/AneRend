#pragma once

#include <cstdint>

namespace render {

typedef std::uint32_t ModelId;
typedef std::uint32_t MeshId;
typedef std::uint32_t RenderableId;
typedef std::uint32_t AnimationId;
typedef std::uint32_t MaterialId;
typedef std::uint32_t SkeletonId;

const std::uint32_t INVALID_ID = 0;

struct IDGenerator
{
  static ModelId genModelId() { return ++_currentModelId; }
  static MeshId genMeshId() { return ++_currentMeshId; }
  static RenderableId genRenderableId() { return ++_currentRenderableId; }
  static AnimationId genAnimationId() { return ++_currentAnimationId; }
  static MaterialId genMaterialId() { return ++_currentMaterialId; }
  static SkeletonId genSkeletonId() { return ++_currentSkeletonId; }

private:
  static ModelId _currentModelId;
  static MeshId _currentMeshId;
  static RenderableId _currentRenderableId;
  static AnimationId _currentAnimationId;
  static MaterialId _currentMaterialId;
  static SkeletonId _currentSkeletonId;
};

}