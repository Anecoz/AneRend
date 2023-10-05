#pragma once

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
  std::vector<Channel> _channels;
};

}