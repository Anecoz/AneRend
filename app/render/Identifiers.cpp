#include "Identifiers.h"

#include <numeric>

namespace render
{

AnimationId IDGenerator::_currentAnimationId = INVALID_ID;
AnimatorId IDGenerator::_currentAnimatorId = INVALID_ID;
ModelId IDGenerator::_currentModelId = INVALID_ID;
MeshId IDGenerator::_currentMeshId = INVALID_ID;
RenderableId IDGenerator::_currentRenderableId = INVALID_ID;
MaterialId IDGenerator::_currentMaterialId = INVALID_ID;
SkeletonId IDGenerator::_currentSkeletonId = INVALID_ID;

template <typename T>
std::vector<T> genRange(std::size_t num, T& start)
{
  std::vector<T> out(num);

  std::iota(out.begin(), out.end(), start + 1);
  start += (T)num;

  return out;
}

std::vector<ModelId> IDGenerator::genModelIdRange(std::size_t num)
{
  return genRange(num, _currentModelId);
}

std::vector<MeshId> IDGenerator::genMeshIdRange(std::size_t num)
{
  return genRange(num, _currentMeshId);
}

std::vector<RenderableId> IDGenerator::genRenderableIdRange(std::size_t num)
{
  return genRange(num, _currentRenderableId);
}

std::vector<AnimationId> IDGenerator::genAnimationIdRange(std::size_t num)
{
  return genRange(num, _currentAnimationId);
}

std::vector<AnimatorId> IDGenerator::genAnimatorIdRange(std::size_t num)
{
  return genRange(num, _currentAnimatorId);
}

std::vector<MaterialId> IDGenerator::genMaterialIdRange(std::size_t num)
{
  return genRange(num, _currentMaterialId);
}

std::vector<SkeletonId> IDGenerator::genSkeletonIdRange(std::size_t num)
{
  return genRange(num, _currentSkeletonId);
}

IdentifiersState IDGenerator::getState()
{
  IdentifiersState state {
    _currentModelId,
    _currentMeshId,
    _currentRenderableId,
    _currentAnimationId,
    _currentAnimatorId,
    _currentMaterialId,
    _currentSkeletonId
  };

  return state;
}

void IDGenerator::setState(IdentifiersState state)
{
  _currentModelId = state._modelIdState;
  _currentMeshId = state._meshIdState;
  _currentRenderableId = state._renderableIdState;
  _currentAnimationId = state._animationIdState;
  _currentAnimatorId = state._animatorIdState;
  _currentMaterialId = state._materialIdState;
  _currentSkeletonId = state._skeletonIdState;
}

}