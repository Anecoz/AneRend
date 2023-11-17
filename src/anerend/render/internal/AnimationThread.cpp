#include "AnimationThread.h"

#include <chrono>

namespace render::internal {

namespace {

void calcSkelParentsAndChildren(render::anim::Skeleton& skele)
{
  // Set children
  for (auto& joint : skele._joints) {
    joint._children.clear();
    joint._parent = nullptr;

    for (auto childId : joint._childrenInternalIds) {
      for (auto& child : skele._joints) {
        if (child._internalId == childId) {
          joint._children.emplace_back(&child);
          break;
        }
      }
    }
  }

  // Set parents
  for (auto& joint : skele._joints) {
    for (auto& parent : skele._joints) {
      for (auto childIdInParent : parent._childrenInternalIds) {
        if (childIdInParent == joint._internalId) {
          joint._parent = &parent;
          break;
        }
      }
      if (joint._parent != nullptr) {
        // We found a parent and broke inner loop, break
        break;
      }
    }
  }
}

}


AnimationThread::AnimationThread()
  : _shutdown(false)
{
  _t = std::thread([this]() {updateThread(); });
}

AnimationThread::~AnimationThread()
{
  _shutdown.store(true);
  _t.join();
}

// Note: This will be called very very often.
std::vector<render::anim::Skeleton> AnimationThread::getCurrentSkeletons()
{
  std::unique_lock<std::mutex> lock(_skelMtx);
  return _currentSkeletons;
}

// Note: the add/removes down here will not be called very often at all.
void AnimationThread::addAnimation(render::anim::Animation&& anim)
{
  std::unique_lock<std::mutex> lock(_mtx);
  _currentAnimations.emplace_back(std::move(anim));
}

void AnimationThread::addAnimator(render::asset::Animator&& animator)
{
  std::unique_lock<std::mutex> lock(_mtx);

  // Check that the animation and skeleton exist
  bool animFound = false;
  bool skeleFound = false;

  render::anim::Animation animCopy;
  render::anim::Skeleton skeleCopy;

  for (auto& anim : _currentAnimations) {
    if (anim._id == animator._animId) {
      animFound = true;
      animCopy = anim;
      break;
    }
  }

  if (!animFound) {
    printf("Cannot add animator, animation %s doesn't exist!\n", animator._animId.str().c_str());
    return;
  }

  lock.unlock();
  {
    // Try pending skeletons
    std::unique_lock<std::mutex> skeleLock(_pendingSkelMtx);
    for (auto& skel : _pendingAddedSkeletons) {
      if (skel._id == animator._skeleId) {
        skeleFound = true;
        skeleCopy = skel;
        break;
      }
    }
  }

  if (!skeleFound) {
    // try current skeletons
    std::unique_lock<std::mutex> skelLock(_skelMtx);
    for (auto& skel : _currentSkeletons) {
      if (skel._id == animator._skeleId) {
        skeleFound = true;
        skeleCopy = skel;
        break;
      }
    }
  }

  if (!skeleFound) {
    printf("Cannot add animator, skeleton %s doesn't exist!\n", animator._skeleId.str().c_str());
    return;
  }

  auto animatorCopy = animator;

  lock.lock();
  _currentAnimators.emplace_back(std::move(animator));
  lock.unlock();

  auto fut = std::async(std::launch::async, &AnimationThread::addInternalAnimator, 
    this, animatorCopy, std::move(animCopy), std::move(skeleCopy));

  std::lock_guard<std::mutex> animLock(_pendingAnimatorsMtx);
  _pendingInternalAnimators.emplace_back(std::move(fut));
}

void AnimationThread::addSkeleton(render::anim::Skeleton&& skele)
{
  std::unique_lock<std::mutex> lock(_pendingSkelMtx);
  _pendingAddedSkeletons.emplace_back(std::move(skele));
}

void AnimationThread::updateAnimator(render::asset::Animator&& animator)
{
  std::unique_lock<std::mutex> lock(_mtx);
  for (auto it = _currentInternalAnimators.begin(); it != _currentInternalAnimators.end(); ++it) {
    if (it->_animatorId == animator._id) {

      // If same skeleton and animation, we can simply update here
      if (it->_animId == animator._animId && it->_skeleId == animator._skeleId) {

        // Did state change?
        if (animator._state != it->_animator.state()) {
          if (animator._state == render::asset::Animator::State::Playing) {
            it->_animator.play();
          }
          else if (animator._state == render::asset::Animator::State::Stopped) {
            it->_animator.stop();
          }
          else if (animator._state == render::asset::Animator::State::Paused) {
            it->_animator.pause();
          }
        }

        // Did multiplier change?
        if (std::abs(animator._playbackMultiplier - it->_animator.playbackMultipler()) > 0.001) {
          it->_animator.setPlaybackMultiplier(animator._playbackMultiplier);
        }
      }
      else {
        // Remove and re-add...
        _currentInternalAnimators.erase(it);
        lock.unlock();
        addAnimator(std::move(animator));
        return;
      }
    }
  }
}

void AnimationThread::removeAnimation(util::Uuid animId)
{
  std::unique_lock<std::mutex> lock(_mtx);
  for (auto it = _currentAnimations.begin(); it != _currentAnimations.end(); ++it) {
    if (it->_id == animId) {
      _currentAnimations.erase(it);
      return;
    }
  }
}

void AnimationThread::removeAnimator(util::Uuid animatorId)
{
  std::unique_lock<std::mutex> lock(_mtx);
  for (auto it = _currentInternalAnimators.begin(); it != _currentInternalAnimators.end(); ++it) {
    if (it->_animatorId == animatorId) {
      _currentInternalAnimators.erase(it);
      break;
    }
  }

  for (auto it = _currentAnimators.begin(); it != _currentAnimators.end(); ++it) {
    if (it->_id == animatorId) {
      _currentAnimators.erase(it);
      break;
    }
  }
}

void AnimationThread::removeSkeleton(util::Uuid skeleId)
{
  std::unique_lock<std::mutex> lock(_skelMtx);
  for (auto it = _currentSkeletons.begin(); it != _currentSkeletons.end(); ++it) {
    if (it->_id == skeleId) {
      _currentSkeletons.erase(it);
      break;
    }
  }
}

void AnimationThread::updateThread()
{
  auto lastUpdate = std::chrono::high_resolution_clock::now();
  while (!_shutdown.load()) {
    // Check pending animators
    std::vector<std::pair<AnimatorData, render::anim::Animation>> completedAnimators;
    {
      std::lock_guard<std::mutex> animLock(_pendingAnimatorsMtx);
      for (auto it = _pendingInternalAnimators.begin(); it != _pendingInternalAnimators.end();) {
        if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
          completedAnimators.emplace_back(std::move(it->get()));
          it = _pendingInternalAnimators.erase(it);
        }
        else {
          ++it;
        }
      }
    }

    if (!completedAnimators.empty()) {
      // Replace the old animation and add Animator
      std::unique_lock<std::mutex> lock(_mtx);
      for (auto& newAnim : completedAnimators) {

        // Replace anim
        for (auto& anim : _currentAnimations) {
          if (anim._id == newAnim.second._id) {
            anim = std::move(newAnim.second);
            break;
          }
        }

        // Add animator
        _currentInternalAnimators.emplace_back(std::move(newAnim.first));
      }
    }

    // Check pending skeletons
    std::vector<render::anim::Skeleton> addedSkeletons;
    {
      std::lock_guard<std::mutex> lock(_pendingSkelMtx);
      std::swap(addedSkeletons, _pendingAddedSkeletons);
    }

    if (!addedSkeletons.empty()) {
      std::lock_guard<std::mutex> lock(_skelMtx);
      for (auto& addedSkel : addedSkeletons) {
        _currentSkeletons.emplace_back(std::move(addedSkel));
        calcSkelParentsAndChildren(_currentSkeletons.back());
      }
    }

    // Run through all current animators
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = now - lastUpdate;
    lastUpdate = now;

    double deltaSeconds = std::chrono::duration<double>(elapsed).count();

    std::vector<render::anim::Skeleton> currentSkeletons;
    {
      std::lock_guard<std::mutex> lock(_skelMtx);
      currentSkeletons = _currentSkeletons;
    }

    {
      std::unique_lock<std::mutex> lock(_mtx);
      for (auto& animator : _currentInternalAnimators) {
        render::anim::Animation* animp = nullptr;
        render::anim::Skeleton* skelep = nullptr;

        for (auto& anim : _currentAnimations) {
          if (anim._id == animator._animId) {
            animp = &anim;
            break;
          }
        }

        for (auto& skele : currentSkeletons) {
          if (skele._id == animator._skeleId) {
            skelep = &skele;
            break;
          }
        }

        if (!animp || !skelep) {
          printf("Cannot connect anim %s with skele %s, one of them doesn't exist!\n", animator._animId.str().c_str(), animator._skeleId.str().c_str());
          continue;
        }

        animator._animator.update(*animp, *skelep, deltaSeconds);
        //animator._animator.updateNoPreCalc(*animp, *skelep, deltaSeconds);
      }
    }

    // Swap local skeletons with class wide
    {
      std::lock_guard<std::mutex> lock(_skelMtx);
      std::swap(_currentSkeletons, currentSkeletons);
    }

    // Allow some access to _mtx.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

std::pair<AnimationThread::AnimatorData, render::anim::Animation> AnimationThread::addInternalAnimator(
  render::asset::Animator animatorAsset,
  render::anim::Animation anim, 
  render::anim::Skeleton skele)
{
  anim::Animator animator{};
  animator.init(anim, skele);
  animator.play();
  animator.precalculateAnimationFrames(anim, skele);
  animator.setPlaybackMultiplier(animatorAsset._playbackMultiplier);
  animator.stop();

  switch (animatorAsset._state) {
  case render::asset::Animator::State::Playing:
    animator.play();
    break;
  case render::asset::Animator::State::Stopped:
    animator.stop();
    break;
  case render::asset::Animator::State::Paused:
    animator.pause();
    break;
  }

  AnimatorData animData{};
  animData._animator = std::move(animator);
  animData._animId = anim._id;
  animData._skeleId = skele._id;
  animData._animatorId = animatorAsset._id;

  return { animData, anim };
}

}