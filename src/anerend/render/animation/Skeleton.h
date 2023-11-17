#pragma once

#include "Joint.h"

#include "../../util/Uuid.h"

#include <optional>
#include <string>
#include <vector>

namespace render::anim {

struct Skeleton
{
  util::Uuid _id = util::Uuid::generate();

  std::string _name;

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