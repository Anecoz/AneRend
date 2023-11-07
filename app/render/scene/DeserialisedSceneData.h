#pragma once

#include "../Identifiers.h"

#include <memory>

namespace render::scene
{
class Scene;

struct DeserialisedSceneData
{
  std::unique_ptr<render::scene::Scene> _scene = nullptr;
  IdentifiersState _idState;
};

}