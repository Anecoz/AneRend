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
std::vector<anim::Skeleton> AnimationThread::getCurrentSkeletons()
{
  std::unique_lock<std::mutex> lock(_skelMtx);
  return _currentSkeletons;
}

// Note: the add/removes down here will not be called very often at all.
void AnimationThread::addAnimation(anim::Animation&& anim)
{
  std::unique_lock<std::mutex> lock(_mtx);
  _currentAnimations.emplace_back(std::move(anim));
}

void AnimationThread::addSkeleton(anim::Skeleton&& skele)
{
  std::unique_lock<std::mutex> lock(_pendingSkelMtx);
  _pendingAddedSkeletons.emplace_back(std::move(skele));
}

void AnimationThread::removeAnimation(render::AnimationId animId)
{
  {
    std::unique_lock<std::mutex> lock(_mtx);
    for (auto it = _currentAnimations.begin(); it != _currentAnimations.end(); ++it) {
      if (it->_id == animId) {
        _currentAnimations.erase(it);
        return;
      }
    }
  }

  // Remove animators with this anim in them
  {
    std::unique_lock<std::mutex> lock(_mtx);
    for (auto it = _currentAnimators.begin(); it != _currentAnimators.end(); ++it) {
      if (it->_animId == animId) {
        _currentAnimators.erase(it);
        return;
      }
    }
  }
}

void AnimationThread::removeSkeleton(render::SkeletonId skeleId)
{
  {
    std::unique_lock<std::mutex> lock(_skelMtx);
    for (auto it = _currentSkeletons.begin(); it != _currentSkeletons.end(); ++it) {
      if (it->_id == skeleId) {
        _currentSkeletons.erase(it);
        break;
      }
    }
  }

  // Remove animators with this skele in them
  {
    std::unique_lock<std::mutex> lock(_mtx);
    for (auto it = _currentAnimators.begin(); it != _currentAnimators.end(); ++it) {
      if (it->_skeleId == skeleId) {
        _currentAnimators.erase(it);
        return;
      }
    }
  }

}

// Note: Should not be called often
void AnimationThread::connect(render::AnimationId animId, render::SkeletonId skeleId)
{
  // Check if this connection already exists, and in that case early exit
  std::unique_lock<std::mutex> lock(_mtx);
  for (auto& animator : _currentAnimators) {
    if (animator._animId == animId && animator._skeleId == skeleId) {
      return;
    }
  }

  std::unique_lock<std::mutex> skelLock(_skelMtx);
  anim::Animation* animp = nullptr;
  anim::Skeleton* skelep = nullptr;

  for (auto& anim : _currentAnimations) {
    if (anim._id == animId) {
      animp = &anim;
      break;
    }
  }

  for (auto& skele : _currentSkeletons) {
    if (skele._id == skeleId) {
      skelep = &skele;
      break;
    }
  }

  if (!animp || !skelep) {
    lock.unlock();
    skelLock.unlock();
    printf("Cannot connect anim %u with skele %u, one of them doesn't exist!\n", animId, skeleId);
    return;
  }

  anim::Animation animCopy = *animp;
  anim::Skeleton skeleCopy = *skelep;

  lock.unlock();
  skelLock.unlock();

  auto fut = std::async(std::launch::async, &AnimationThread::addAnimator, this, std::move(animCopy), std::move(skeleCopy));

  std::lock_guard<std::mutex> animLock(_pendingAnimatorsMtx);
  _pendingAnimators.emplace_back(std::move(fut));
}

void AnimationThread::disconnect(render::AnimationId animId, render::SkeletonId skeleId)
{
  std::unique_lock<std::mutex> lock(_mtx);
  for (auto it = _currentAnimators.begin(); it != _currentAnimators.end(); ++it) {
    if (it->_animId == animId && it->_skeleId == skeleId) {
      _currentAnimators.erase(it);
      return;
    }
  }
}

void AnimationThread::playAnimation(render::SkeletonId skeleId)
{
  // TODO
}

void AnimationThread::pauseAnimation(render::SkeletonId skeleId)
{
  // TODO
}

void AnimationThread::stopAnimation(render::SkeletonId skeleId)
{
  // TODO
}

void AnimationThread::updateThread()
{
  auto lastUpdate = std::chrono::high_resolution_clock::now();
  while (!_shutdown.load()) {
    // Check pending animators
    std::vector<std::pair<AnimatorData, anim::Animation>> completedAnimators;
    {
      std::lock_guard<std::mutex> animLock(_pendingAnimatorsMtx);
      for (auto it = _pendingAnimators.begin(); it != _pendingAnimators.end();) {
        if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
          completedAnimators.emplace_back(std::move(it->get()));
          it = _pendingAnimators.erase(it);
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
        _currentAnimators.emplace_back(std::move(newAnim.first));
      }
    }

    // Check pending skeletons
    std::vector<anim::Skeleton> addedSkeletons;
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

    std::vector<anim::Skeleton> currentSkeletons;
    {
      std::lock_guard<std::mutex> lock(_skelMtx);
      currentSkeletons = _currentSkeletons;
    }

    {
      std::unique_lock<std::mutex> lock(_mtx);
      for (auto& animator : _currentAnimators) {
        anim::Animation* animp = nullptr;
        anim::Skeleton* skelep = nullptr;

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
          printf("Cannot connect anim %u with skele %u, one of them doesn't exist!\n", animator._animId, animator._skeleId);
          continue;
        }

        animator._animator.update(*animp, *skelep, deltaSeconds);
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

std::pair<AnimationThread::AnimatorData, anim::Animation> AnimationThread::addAnimator(anim::Animation anim, anim::Skeleton skele)
{
  anim::Animator animator{};
  animator.init(anim, skele);
  animator.play();
  animator.precalculateAnimationFrames(anim, skele);

  AnimatorData animData{};
  animData._animator = std::move(animator);
  animData._animId = anim._id;
  animData._skeleId = skele._id;

  return { animData, anim };
}

}