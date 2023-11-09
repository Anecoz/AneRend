#pragma once

#include "../animation/Animation.h"
#include "../animation/Skeleton.h"
#include "../asset/Animator.h"

namespace render::internal::anim {

class Animator
{
public:
  Animator();
  ~Animator() = default;

  // TODO: Thread safety is out the window
  //Animator(Animation animation, Skeleton* skeleton);

  void init(render::anim::Animation& anim, render::anim::Skeleton& skeleton);

  // This will go through the animation at the specified framerate (in Hz) and pre-calculate frames.
  // Once this is done, each update call will simply snap to the closest pre-calculated frame.
  void precalculateAnimationFrames(render::anim::Animation& anim, render::anim::Skeleton& skeleton, unsigned framerate = 60);

  void play();
  void pause();
  void stop();
  void setPlaybackMultiplier(float multiplier);

  render::asset::Animator::State state() { return _state; }
  bool isPlaying() { return _state == render::asset::Animator::State::Playing; }
  bool isPaused() { return _state == render::asset::Animator::State::Paused; }
  bool isStopped() { return _state == render::asset::Animator::State::Stopped; }
  float playbackMultipler() { return _playbackMultiplier; }
  
  void update(render::anim::Animation& anim, render::anim::Skeleton& skeleton, double delta);

  // This doesn't use the precalculated frames, but lerps joints on the fly.
  void updateNoPreCalc(render::anim::Animation& anim, render::anim::Skeleton& skeleton, double delta);

private:
  //Animation _animation;
  //Skeleton* _skeleton = nullptr;

  render::asset::Animator::State _state;

  double _animationTime = 0.0;
  double _maxTime = 0.0;
  float _playbackMultiplier = 1.0f;

  std::size_t findClosestPrecalcTime(render::anim::Animation& anim, double time);
};

}