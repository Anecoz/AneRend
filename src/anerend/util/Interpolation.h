#pragma once

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/glm.hpp>

#include <cmath>
#include <numbers>

namespace util::interp {

enum class Easing
{
  Linear,
  InOutSine
};

// Easing functions from here: https://easings.net/
static double easeInOutSine(double x)
{
  return -(std::cos(std::numbers::pi_v<double> *x) - 1.0) / 2.0;
}

static glm::mat4 lerpTransforms(const glm::mat4& m0, const glm::mat4& m1, double factor, Easing easing = Easing::Linear)
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

  double easedFactor = factor;
  if (easing == Easing::InOutSine) {
    easedFactor = easeInOutSine(factor);
  }

  q_out = glm::slerp(q0, q1, (float)easedFactor);

  // Always lerp up-axis linearly (?)
  float y = (1.0f - (float)factor) * t0.y + (float)factor * t1.y;

  s_out = (1.0f - (float)easedFactor) * s0 + (float)easedFactor * s1;
  t_out = (1.0f - (float)easedFactor) * t0 + (float)easedFactor * t1;
  t_out.y = y;

  glm::mat4 rotMat = glm::toMat4(q_out);
  glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), s_out);
  glm::mat4 transMat = glm::translate(glm::mat4(1.0f), t_out);

  out = transMat * rotMat * scaleMat;

  return out;
}

}