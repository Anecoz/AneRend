#pragma once

#include "../../util/Uuid.h"

#include <memory>

namespace render::scene
{
class Scene;

struct DeserialisedSceneData
{
  std::unique_ptr<render::scene::Scene> _scene = nullptr;
};

}