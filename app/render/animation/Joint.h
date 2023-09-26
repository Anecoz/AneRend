#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace render::anim {

struct Joint
{
  Joint* _parent = nullptr;
  std::vector<Joint*> _children;

  glm::mat4 _globalTransform;
  glm::mat4 _localTransform;
  glm::mat4 _originalLocalTransform;
  glm::mat4 _inverseBindMatrix = glm::mat4(1.0f);

  // Internally used id, DO NOT use as a Joint ID for vertex attributes
  int _internalId;
  std::vector<int> _childrenInternalIds;
};

}