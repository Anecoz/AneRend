#pragma once

#include "../render/Identifiers.h"
#include "../render/Camera.h"

namespace render::scene { class Scene; }

namespace logic {

struct AneditContext
{
  virtual ~AneditContext() {}

  virtual render::scene::Scene& scene() = 0;

  virtual render::RenderableId& selectedRenderable() = 0;

  virtual render::Camera& camera() = 0;

protected:
  render::RenderableId _selectedRenderable = render::INVALID_ID;
};

}