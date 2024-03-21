#include "BehaviourHelper.h"

#include "IBehaviour.h"
#include "../render/scene/Scene.h"

namespace behaviour {

void BehaviourHelper::setScene(IBehaviour* behaviour, render::scene::Scene* scene)
{
  behaviour->_scene = scene;
}

void BehaviourHelper::setMe(IBehaviour* behaviour, const util::Uuid& me)
{
  behaviour->_me = me;
}

}