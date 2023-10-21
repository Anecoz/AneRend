#pragma once

#include "../Identifiers.h"

#include <glm/glm.hpp>
#include <utility>
#include <vector>

namespace render::anim {

enum class ChannelPath
{
  Rotation,
  Translation,
  Scale
};

struct Channel
{
  // Internal id of joint that this channel animates
  int _internalId;
  ChannelPath _path;

  // Input and output must match each other
  std::vector<float> _inputTimes;

  // trans and scale use first three components, quat uses all four
  std::vector<glm::vec4> _outputs;
};

struct Animation
{
  render::AnimationId _id = INVALID_ID;
  std::vector<Channel> _channels;

  // Keyframes that are pre-calculated from the channels. Typically an Animator will set these.
  struct InterpolatedKeyframe
  {
    // <internalId, globalTransform>
    std::vector<std::pair<int, glm::mat4>> _joints;
  };

  // pair.first is timestamp when the interpolated keyframe is active
  std::vector<std::pair<double, InterpolatedKeyframe>> _keyframes;
};

}