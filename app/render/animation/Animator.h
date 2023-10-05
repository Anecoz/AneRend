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

  // This will go through the animation at the specified framerate (in Hz) and pre-calculate frames.
  // Once this is done, each update call will simply snap to the closest pre-calculated frame.
  void precalculateAnimationFrames(unsigned framerate = 60);

  void play();
  void pause();
  void stop();
  
  void update(double delta);

  // This doesn't use the precalculated frames, but lerps joints on the fly.
  void updateNoPreCalc(double delta);

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

  std::size_t findClosestPrecalcTime(double time);

  // Keyframes that are pre-calculated from the input animation
  struct InterpolatedKeyframe
  {
    // <internalId, globalTransform>
    std::vector<std::pair<int, glm::mat4>> _joints;
  };

  // pair.first is timestamp when the interpolated keyframe is active
  std::vector<std::pair<double, InterpolatedKeyframe>> _keyframes;
};

}