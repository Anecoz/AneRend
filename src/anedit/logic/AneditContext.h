#pragma once

namespace render::scene { class Scene; }

namespace logic {

struct AneditContext
{
  virtual ~AneditContext() {}

  virtual render::scene::Scene& scene() = 0;
};

}