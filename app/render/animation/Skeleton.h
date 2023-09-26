#pragma once

#include "Joint.h"

#include <optional>
#include <vector>

namespace render::anim {

struct Skeleton
{
  void calcGlobalTransforms();

  void reset();

  explicit operator bool() const
  {
    return !_joints.empty();
  }

  bool _nonJointRoot = false; // If true, the first entry in the _joints vec is a pivot root joint, not referenced by vertex attributes
  std::vector<Joint> _joints;
};

}