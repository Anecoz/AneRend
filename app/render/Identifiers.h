#pragma once

#include <cstdint>
#include <vector>

namespace render {

typedef std::uint32_t ModelId;
typedef std::uint32_t MeshId;
typedef std::uint32_t RenderableId;
typedef std::uint32_t AnimationId;
typedef std::uint32_t AnimatorId;
typedef std::uint32_t MaterialId;
typedef std::uint32_t SkeletonId;

const std::uint32_t INVALID_ID = 0;

struct IDGenerator
{
  static ModelId genModelId() { return ++_currentModelId; }
  static MeshId genMeshId() { return ++_currentMeshId; }
  static RenderableId genRenderableId() { return ++_currentRenderableId; }
  static AnimationId genAnimationId() { return ++_currentAnimationId; }
  static AnimatorId genAnimatorId() { return ++_currentAnimatorId; }
  static MaterialId genMaterialId() { return ++_currentMaterialId; }
  static SkeletonId genSkeletonId() { return ++_currentSkeletonId; }

  static std::vector<ModelId> genModelIdRange(std::size_t num);
  static std::vector<MeshId> genMeshIdRange(std::size_t num);
  static std::vector<RenderableId> genRenderableIdRange(std::size_t num);
  static std::vector<AnimationId> genAnimationIdRange(std::size_t num);
  static std::vector<AnimatorId> genAnimatorIdRange(std::size_t num);
  static std::vector<MaterialId> genMaterialIdRange(std::size_t num);
  static std::vector<SkeletonId> genSkeletonIdRange(std::size_t num);

private:
  static ModelId _currentModelId;
  static MeshId _currentMeshId;
  static RenderableId _currentRenderableId;
  static AnimationId _currentAnimationId;
  static AnimationId _currentAnimatorId;
  static MaterialId _currentMaterialId;
  static SkeletonId _currentSkeletonId;
};

}