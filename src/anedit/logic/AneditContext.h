#pragma once

#include <util/Uuid.h>
#include <render/Camera.h>

namespace render::scene { class Scene; }

namespace logic {

struct AneditContext
{
  virtual ~AneditContext() {}

  virtual render::scene::Scene& scene() = 0;

  virtual util::Uuid& selectedRenderable() = 0;

  virtual render::Camera& camera() = 0;

protected:
  util::Uuid _selectedRenderable;
};

}