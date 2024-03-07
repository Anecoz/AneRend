#include "AnimationUpdater.h"

#include "../scene/Scene.h"
#include "../asset/AssetCollection.h"

namespace render::anim
{

AnimationUpdater::AnimationUpdater(render::scene::Scene* scene, render::asset::AssetCollection* assColl)
  : _scene(scene)
  , _assColl(assColl)
{}

void AnimationUpdater::update(double delta)
{
  // TODO: Abstract?
  auto view = _scene->registry().getEnttRegistry().view<component::Animator>();
  for (auto entity : view) {
    auto nodeId = _scene->registry().reverseLookup(entity);

    // if explicitly not paged, don't bother
    if (_scene->registry().hasComponent<component::PageStatus>(nodeId)) {
      if (!_scene->registry().getComponent<component::PageStatus>(nodeId)._paged) {
        continue;
      }
    }

    auto& animatorComp = _scene->registry().getComponent<component::Animator>(nodeId);

    if (!_cachedAnimations.contains(animatorComp._currentAnimation)) {
      _cachedAnimations[animatorComp._currentAnimation] = _assColl->getAnimationBlocking(animatorComp._currentAnimation);
    }

    Animation& animation = _cachedAnimations[animatorComp._currentAnimation];

    // should assert this
    auto& skeleComp = _scene->registry().getComponent<component::Skeleton>(nodeId);

    // Do we not have an animator yet?
    if (_animators.find(nodeId) == _animators.end()) {
      internal::Animator animator{};
      animator.init(animation, skeleComp);
      animator.precalculateAnimationFrames(_scene, animation, skeleComp);
      _animators[nodeId] = std::move(animator);
    }

    auto& animator = _animators[nodeId];

    // Update
    animator.setPlaybackMultiplier(animatorComp._playbackMultiplier);

    if (animatorComp._state != animator.state()) {
      animator.setState(animatorComp._state);
    }

    if (animator.initedAnimation() != animatorComp._currentAnimation) {
      // Reinit
      animator.init(animation, skeleComp);
      animator.precalculateAnimationFrames(_scene, animation, skeleComp);
    }

    animator.update(_scene, animation, skeleComp, delta);
  }
}

}