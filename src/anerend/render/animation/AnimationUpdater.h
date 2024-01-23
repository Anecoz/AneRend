#pragma once

#include "internal/Animator.h"

#include <unordered_map>

namespace render::scene { class Scene; }

namespace render::anim {

class AnimationUpdater
{
public:
  AnimationUpdater() = default;
  AnimationUpdater(render::scene::Scene* scene);
  ~AnimationUpdater() = default;

  // No move or copy
  AnimationUpdater(const AnimationUpdater&) = delete;
  AnimationUpdater(AnimationUpdater&&) = delete;
  AnimationUpdater& operator=(const AnimationUpdater&) = delete;
  AnimationUpdater& operator=(AnimationUpdater&&) = delete;

  void setScene(render::scene::Scene* scene) { _scene = scene; }

  void update(double delta);

private:
  render::scene::Scene* _scene = nullptr;

  std::unordered_map<util::Uuid, internal::Animator> _animators;
};

}