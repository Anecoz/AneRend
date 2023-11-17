#pragma once

#include "../animation/Skeleton.h"
#include "../animation/Animation.h"
#include "../asset/Animator.h"
#include "Animator.h"

#include <atomic>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

namespace render::internal {

class AnimationThread
{
public:
  AnimationThread();
  ~AnimationThread();

  // No copy or move
  AnimationThread(const AnimationThread&) = delete;
  AnimationThread(AnimationThread&&) = delete;
  AnimationThread& operator=(const AnimationThread&) = delete;
  AnimationThread& operator=(AnimationThread&&) = delete;

  // Copy current state of skeleton
  // The state is guaranteed to be complete, i.e. a full keyframe is applied to all joints.
  std::vector<render::anim::Skeleton> getCurrentSkeletons();

  // These take ownership. Caller has to keep the id to reference these later.
  void addAnimation(render::anim::Animation&& anim);
  void addAnimator(render::asset::Animator&& animator);
  void addSkeleton(render::anim::Skeleton&& skele);

  void updateAnimator(render::asset::Animator&& animator);

  void removeAnimation(util::Uuid animId);
  void removeAnimator(util::Uuid animatorId);
  void removeSkeleton(util::Uuid skeleId);

private:
  struct AnimatorData
  {
    render::internal::anim::Animator _animator;
    util::Uuid _animId;
    util::Uuid _skeleId;
    util::Uuid _animatorId;
  };

  std::vector<render::anim::Skeleton> _currentSkeletons;
  std::vector<render::anim::Animation> _currentAnimations;
  std::vector<render::asset::Animator> _currentAnimators;
  std::vector<AnimatorData> _currentInternalAnimators;
  std::vector<std::future<std::pair<AnimatorData, render::anim::Animation>>> _pendingInternalAnimators;
  std::vector<render::anim::Skeleton> _pendingAddedSkeletons;

  // This mutex protects all the _current* vectors above except the skeletons.
  std::mutex _mtx;

  // Protects the _currentSkeletons.
  std::mutex _skelMtx;

  // Protects pending skeletons.
  std::mutex _pendingSkelMtx;

  // Protects the pending animators
  std::mutex _pendingAnimatorsMtx;

  std::atomic<bool> _shutdown;
  std::thread _t;

  void updateThread();

  // This is launched asynchronously and calculates the interpolated precalc frames.
  // Also returns an updated Animation object containing the precalculated frames.
  std::pair<AnimatorData, render::anim::Animation> addInternalAnimator(render::asset::Animator animator, render::anim::Animation anim, render::anim::Skeleton skele);
};

}