#pragma once

#include "../../util/Uuid.h"

namespace render::scene {

/*
* These Nodes can be compared to an entity in an ECS.
* You can add components to the scene::Nodes via the registry.
*/
struct Node
{
  util::Uuid _id = util::Uuid::generate();

  util::Uuid _parent;
  std::vector<util::Uuid> _children;
};

}