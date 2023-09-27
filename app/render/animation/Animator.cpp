#include "Animator.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace render::anim {

namespace
{

glm::mat4 lerpTransforms(const glm::mat4& m0, const glm::mat4& m1, double factor)
{
  glm::mat4 out{ 1.0f };

  // Decompose both matrices and lerp scale, trans and rot separately, then combine again
  glm::vec3 s0, s1;
  glm::quat q0, q1;
  glm::vec3 t0, t1;

  glm::vec3 unused_0;
  glm::vec4 unused_1;

  glm::decompose(m0, s0, q0, t0, unused_0, unused_1);
  glm::decompose(m1, s1, q1, t1, unused_0, unused_1);

  glm::vec3 s_out{ 1.0f };
  glm::quat q_out{ 1.0f, 0.0f, 0.0f, 0.0f };
  glm::vec3 t_out{ 0.0f };

  q_out = glm::slerp(q0, q1, (float)factor);
  s_out = (1.0f - (float)factor) * s0 + (float)factor * s1;
  t_out = (1.0f - (float)factor) * t0 + (float)factor * t1;

  glm::mat4 rotMat = glm::toMat4(q_out);
  glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), s_out);
  glm::mat4 transMat = glm::translate(glm::mat4(1.0f), t_out);

  out = transMat * rotMat * scaleMat;

  return out;
}

void lerpPath(ChannelPath path, glm::mat4* inputMat, const glm::vec4& v0, const glm::vec4& v1, double factor)
{
  glm::vec3 s;
  glm::quat q;
  glm::vec3 t;

  glm::vec3 unused_0;
  glm::vec4 unused_1;

  glm::decompose(*inputMat, s, q, t, unused_0, unused_1);

  if (path == ChannelPath::Rotation) {
    glm::quat q0{ v0.x, v0.y, v0.z, v0.w };
    glm::quat q1{ v1.x, v1.y, v1.z, v1.w };

    q = glm::slerp(q0, q1, (float)factor);

    //printf("q: %f %f %f %f\n", q.x, q.y, q.z, q.w);
  }
  else if (path == ChannelPath::Translation) {
    glm::vec3 t0{ v0.x, v0.y, v0.z };
    glm::vec3 t1{ v1.x, v1.y, v1.z };

    t = (1.0f - (float)factor) * t0 + (float)factor * t1;
  }
  else if (path == ChannelPath::Scale) {
    glm::vec3 s0{ v0.x, v0.y, v0.z };
    glm::vec3 s1{ v1.x, v1.y, v1.z };

    s = (1.0f - (float)factor) * s0 + (float)factor * s1;
  }

  glm::mat4 rotMat = glm::toMat4(q);
  glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), s);
  glm::mat4 transMat = glm::translate(glm::mat4(1.0f), t);

  *inputMat = transMat * rotMat * scaleMat;
}

std::size_t findIndexForTime(double time, Channel& channel)
{
  if (channel._inputTimes.size() == 1) {
    // Save us some trouble
    return 0;
  }

  for (std::size_t i = 0; i < channel._inputTimes.size(); ++i) {
    float inputTime = channel._inputTimes[i];
    if (inputTime > time) {
      return i-1;
    }
  }

  printf("Could not find animation index for time %lf!\n", time);
  return 0;
}

}

Animator::Animator()
  : _state(State::Stopped)
{
}

Animator::Animator(Animation animation, Skeleton* skeleton)
  : _animation(animation)
  , _skeleton(skeleton)
  , _state(State::Stopped)
{
  // Calculate max time
  _maxTime = 0.0;
  for (auto& channel : animation._channels) {
    for (float time : channel._inputTimes) {
      if (time > _maxTime) {
        _maxTime = time;
      }
    }
  }
}

void Animator::play()
{
  _state = State::Playing;
}

void Animator::pause()
{
  _state = State::Paused;
}

void Animator::stop()
{
  _state = State::Stopped;
  _animationTime = 0.0;
}

void Animator::update(double delta)
{
  if (_state == State::Stopped || _state == State::Paused) {
    return;
  }

  // We're playing
  _animationTime += delta;

  // Do we have to wrap time?
  if (_animationTime >= _maxTime) {
    _animationTime = 0.0;
  }

  _skeleton->reset();

  // Update each channel one after the other
  for (auto& channel : _animation._channels) {
    // Find the indices that encompass the current time
    auto idx = findIndexForTime(_animationTime, channel);

    double factor = 0.0;
    glm::vec4& vec0 = channel._outputs[idx];
    glm::vec4 vec1{ 0.0 };

    if (channel._outputs.size() == 1) {
      vec1 = vec0;
    }
    else {
      vec1 = channel._outputs[idx + 1];

      factor = (_animationTime - channel._inputTimes[idx]) / (channel._inputTimes[idx + 1] - channel._inputTimes[idx]);
    }

    // Find the joint
    Joint* joint = nullptr;
    for (auto& j : _skeleton->_joints) {
      if (j._internalId == channel._internalId) {
        joint = &j;
      }
    }

    lerpPath(channel._path, &joint->_localTransform, vec0, vec1, factor);

    // Update global transforms of skeleton
    _skeleton->calcGlobalTransforms();
  }

}
}