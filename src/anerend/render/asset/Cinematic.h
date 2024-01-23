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
  glm::vec3 _pos;
  glm::quat _orientation;
  glm::vec3 _ypr;

  // TODO: Add things for the projection, like fov
};

struct NodeKeyframe
{
  util::interp::Easing _easing = util::interp::Easing::InOutCubic;
  util::Uuid _id;
  component::PotentialComponents _comps;
};

// Only support a couple of parameters
struct MaterialKeyframe
{
  util::Uuid _id;

  glm::vec4 _emissive;
  glm::vec3 _baseColFactor;
};

// Easier to just put the whole animator in here. It is quite cheap.
struct AnimatorKeyframe
{
  util::Uuid _id;

  component::Animator _animator;
};

struct Cinematic
{
  util::Uuid _id = util::Uuid::generate();

  std::string _name;

  struct Keyframe
  {
    float _time;

    std::optional<CameraKeyframe> _camKF;
    std::vector<NodeKeyframe> _nodeKFs;
    std::vector<MaterialKeyframe> _materialKFs;
    std::vector<AnimatorKeyframe> _animatorKFs;
  };

  std::vector<Keyframe> _keyframes;
};

}