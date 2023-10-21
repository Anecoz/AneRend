#pragma once

#include "../animation/Skeleton.h"
#include "../animation/Animation.h"
#include "../animation/Animator.h"

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
  std::vector<anim::Skeleton> getCurrentSkeletons();

  // These take ownership. Caller has to keep the id to reference these later.
  void addAnimation(anim::Animation&& anim);
  void addSkeleton(anim::Skeleton&& skele);

  void removeAnimation(render::AnimationId animId);
  void removeSkeleton(render::SkeletonId skeleId);

  // Apply animId to skeleId
  void connect(render::AnimationId animId, render::SkeletonId skeleId);

  // I think skeleton is enough...? A skeleton can only have 1 animation associated with it,
  // whereas a single animation can be used with multiple skeletons.
  void playAnimation(render::SkeletonId skeleId);
  void pauseAnimation(render::SkeletonId skeleId);
  void stopAnimation(render::SkeletonId skeleId);

private:
  struct AnimatorData
  {
    anim::Animator _animator;
    render::AnimationId _animId;
    render::SkeletonId _skeleId;
  };

  std::vector<anim::Skeleton> _currentSkeletons;
  std::vector<anim::Animation> _currentAnimations;
  std::vector<AnimatorData> _currentAnimators;
  std::vector<std::future<std::pair<AnimatorData, anim::Animation>>> _pendingAnimators;
  std::vector<anim::Skeleton> _pendingAddedSkeletons;

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
  std::pair<AnimatorData, anim::Animation> addAnimator(anim::Animation anim, anim::Skeleton skele);
};

}