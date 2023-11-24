#pragma once

#include "../../util/Uuid.h"

namespace render::asset {

struct Animator
{
  util::Uuid _id = util::Uuid::generate();

  std::string _name;

  enum class State : std::uint8_t {
    Playing,
    Paused,
    Stopped
  } _state;

  util::Uuid _skeleId;
  util::Uuid _animId;
  float _playbackMultiplier = 1.0f;
};

}