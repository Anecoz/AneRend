#pragma once

#include "../Identifiers.h"

namespace render::asset {

struct Animator
{
  AnimatorId _id = INVALID_ID;

  enum class State : std::uint8_t {
    Playing,
    Paused,
    Stopped
  } _state;

  SkeletonId _skeleId = INVALID_ID;
  AnimationId _animId = INVALID_ID;
  float _playbackMultiplier = 1.0f;
};

}