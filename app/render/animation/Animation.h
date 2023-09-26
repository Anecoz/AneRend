#pragma once

#include <glm/glm.hpp>
#include <utility>
#include <vector>

namespace render::anim {

/*struct Keyframe
{
  // The timepoint in seconds when this keyframe should be "posed"
  float _time;

  // The int corresponds to the internal joint id, and the matrix is the keyframe's transform for that joint
  std::vector<std::pair<int, glm::mat4>> _jointTransforms;

  int _internalId;
};*/

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
  //std::vector<Keyframe> _keyframes;
};

}