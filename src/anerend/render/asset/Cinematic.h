#pragma once

#include "../../util/Uuid.h"
#include "../../util/Interpolation.h"
#include "../../component/Components.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <optional>
#include <string>
#include <vector>

namespace render::asset {

struct CameraKeyframe
{
  util::interp::Easing _easing = util::interp::Easing::InOutCubic;
  float _time;
  glm::vec3 _pos;
  glm::quat _orientation;
  glm::vec3 _ypr;

  // TODO: Add things for the projection, like fov
};

struct NodeKeyframe
{
  util::interp::Easing _easing = util::interp::Easing::InOutCubic;
  float _time;
  util::Uuid _id;
  component::PotentialComponents _comps;
};

// Only support a couple of parameters
struct MaterialKeyframe
{
  util::Uuid _id;
  float _time;

  glm::vec4 _emissive;
  glm::vec3 _baseColFactor;
};

// Easier to just put the whole animator in here. It is quite cheap.
struct AnimatorKeyframe
{
  util::Uuid _id;
  float _time;

  component::Animator _animator;
};

struct Cinematic
{
  util::Uuid _id = util::Uuid::generate();

  std::string _name;

  std::vector<CameraKeyframe> _camKeyframes;
  std::vector<std::vector<NodeKeyframe>> _nodeKeyframes;
  std::vector<MaterialKeyframe> _materialKeyframes;

  float _maxTime = 0.0f;
};

}