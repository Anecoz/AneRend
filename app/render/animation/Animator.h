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
  Animator(Animation animation, Skeleton* skeleton);

  void play();
  void pause();
  void stop();
  
  void update(double delta);

private:
  Animation _animation;
  Skeleton* _skeleton = nullptr;

  enum class State
  {
    Playing,
    Paused,
    Stopped
  } _state;

  double _animationTime = 0.0;

  double _maxTime = 0.0;

  //std::vector<Keyframe*> findKeyframesForTime(double time);
  //void interpolateKeyframes(Keyframe* k0, Keyframe* k1, double factor);
};

}