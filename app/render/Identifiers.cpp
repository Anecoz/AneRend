#include "Identifiers.h"

namespace render
{
  AnimationId IDGenerator::_currentAnimationId = INVALID_ID;
  AnimatorId IDGenerator::_currentAnimatorId = INVALID_ID;
  ModelId IDGenerator::_currentModelId = INVALID_ID;
  MeshId IDGenerator::_currentMeshId = INVALID_ID;
  RenderableId IDGenerator::_currentRenderableId = INVALID_ID;
  MaterialId IDGenerator::_currentMaterialId = INVALID_ID;
  SkeletonId IDGenerator::_currentSkeletonId = INVALID_ID;
}