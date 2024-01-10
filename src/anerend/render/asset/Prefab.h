#pragma once

#include "../../util/Uuid.h"

#include "../../component/Components.h"

#include <string>
#include <glm/glm.hpp>

namespace render::asset {

// A prefab can be used to instantiate a scene node
// There is a child-parent relationship here that will be mirrored by the instances
struct Prefab
{
  util::Uuid _id = util::Uuid::generate();
  std::string _name;

  component::PotentialComponents _comps;

  // Note: The ids refer to parent and children _prefabs_
  util::Uuid _parent;
  std::vector<util::Uuid> _children;
};

}