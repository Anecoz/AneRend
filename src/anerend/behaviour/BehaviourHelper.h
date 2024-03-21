#pragma once

#include "../util/Uuid.h"

namespace render::scene { class Scene; }

namespace behaviour {

class IBehaviour;

class BehaviourHelper
{
public:
  static void setScene(IBehaviour* behaviour, render::scene::Scene* scene);
  static void setMe(IBehaviour* behaviour, const util::Uuid& me);
};

}