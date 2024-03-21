#pragma once

#include "../util/Uuid.h"

namespace render::scene { class Scene; }

namespace behaviour {

class BehaviourHelper;

class IBehaviour
{
public:
  virtual ~IBehaviour() {}

  // Called exactly once when the behaviour starts up.
  virtual void start() = 0;

  // Called every frame from the main thread.
  virtual void update(double delta) = 0;

  virtual const util::Uuid& me() { return _me; }

protected:
  friend class BehaviourHelper;

  util::Uuid _me;
  render::scene::Scene* _scene;
};

}