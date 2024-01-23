#pragma once

#include "../../../component/Components.h"
#include "../Animation.h"

namespace render::scene { class Scene; }

namespace render::anim::internal {

class Animator
{
public:
  Animator() = default;
  ~Animator() = default;

  // Calc max time
  void init(const render::anim::Animation& anim, component::Skeleton& skeleton);
  util::Uuid initedAnimation() const { return _initedAnimation; }

  // This will go through the animation at the specified framerate (in Hz) and pre-calculate frames.
  // Once this is done, each update call will simply snap to the closest pre-calculated frame.
  void precalculateAnimationFrames(render::scene::Scene* scene, render::anim::Animation& anim, component::Skeleton& skeleton, unsigned framerate = 60);

  void play() { _state = component::Animator::State::Playing; }
  void pause() { _state = component::Animator::State::Paused; }
  void stop() { _state = component::Animator::State::Stopped; _animationTime = 0.0; }
  void setPlaybackMultiplier(float multiplier) { _playbackMultiplier = multiplier; }

  component::Animator::State state() const { return _state; }
  void setState(component::Animator::State state) { _state = state; if (isStopped()) _animationTime = 0.0; }
  bool isPlaying() const { return _state == component::Animator::State::Playing; }
  bool isPaused() const { return _state == component::Animator::State::Paused; }
  bool isStopped() const { return _state == component::Animator::State::Stopped; }
  float playbackMultipler() const { return _playbackMultiplier; }

  void update(render::scene::Scene* scene, render::anim::Animation& anim, component::Skeleton& skeleton, double delta);

  // This doesn't use the precalculated frames, but lerps joints on the fly.
  void updateNoPreCalc(render::scene::Scene* scene, render::anim::Animation& anim, component::Skeleton& skeleton, double delta);

private:
  component::Animator::State _state = component::Animator::State::Stopped;

  double _animationTime = 0.0;
  double _maxTime = 0.0;
  float _playbackMultiplier = 1.0f;

  util::Uuid _initedAnimation;

  std::size_t findClosestPrecalcTime(render::anim::Animation& anim, double time);
};

}