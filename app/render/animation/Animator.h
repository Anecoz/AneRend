#pragma once

#include "Animation.h"
#include "Skeleton.h"

namespace render::anim {

class Animator
{
public:
  Animator();
  ~Animator() = default;

  // TODO: Thread safety is out the window
  //Animator(Animation animation, Skeleton* skeleton);

  void init(Animation& anim, Skeleton& skeleton);

  // This will go through the animation at the specified framerate (in Hz) and pre-calculate frames.
  // Once this is done, each update call will simply snap to the closest pre-calculated frame.
  void precalculateAnimationFrames(Animation& anim, Skeleton& skeleton, unsigned framerate = 60);

  void play();
  void pause();
  void stop();
  
  void update(Animation& anim, Skeleton& skeleton, double delta);

  // This doesn't use the precalculated frames, but lerps joints on the fly.
  void updateNoPreCalc(Animation& anim, Skeleton& skeleton, double delta);

private:
  //Animation _animation;
  //Skeleton* _skeleton = nullptr;

  enum class State
  {
    Playing,
    Paused,
    Stopped
  } _state;

  double _animationTime = 0.0;
  double _maxTime = 0.0;

  std::size_t findClosestPrecalcTime(Animation& anim, double time);
};

}