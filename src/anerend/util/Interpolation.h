#pragma once

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/glm.hpp>

#include <cmath>
#include <numbers>

namespace util::interp {

enum class Easing : std::uint8_t
{
  Linear,
  InOutSine,
  InCubic,
  OutCubic,
  InOutCubic,
  InOutElastic,
  OutBounce,
  InOutBounce
};

static std::string easingToStr(Easing easing)
{
  switch (easing) {
  case Easing::InCubic:
    return "In cubic";
  case Easing::InOutBounce:
    return "In Out bounce";
  case Easing::InOutCubic:
    return "In Out cubic";
  case Easing::InOutElastic:
    return "In Out elastic";
  case Easing::InOutSine:
    return "In Out sine";
  case Easing::Linear:
    return "Linear";
  case Easing::OutBounce:
    return "Out bounce";
  case Easing::OutCubic:
    return "Out cubic";
  }

  return "";
}

// Easing functions from here: https://easings.net/
static double easeInOutSine(double x)
{
  return -(std::cos(std::numbers::pi_v<double> *x) - 1.0) / 2.0;
}

// A bit softer than sine version
static double easeInOutCubic(double x)
{
  return x < 0.5 ? 4 * x * x * x : 1 - std::pow(-2 * x + 2, 3) / 2;
}

static double easeInCubic(double x)
{
  return x * x * x;
}

static double easeOutCubic(double x)
{
  return 1 - std::pow(1 - x, 3);
}

// Very bouncy, very hilarious
static double easeInOutElastic(double x)
{
  const auto c5 = (2.0 * std::numbers::pi_v<double>) / 4.5;

  return x == 0.0 ? 0.0 :
    x == 1.0 ? 1.0 :
    x < 0.5 ? -(std::pow(2.0, 20.0 * x - 10.0) * std::sin((20.0 * x - 11.125) * c5)) / 2.0 :
    (std::pow(2.0, -20.0 * x + 10.0) * std::sin((20.0 * x - 11.125) * c5)) / 2.0 + 1.0;
}

static double easeOutBounce(double x)
{
  const auto n1 = 7.5625;
  const auto d1 = 2.75;

  if (x < 1 / d1) {
    return n1 * x * x;
  }
  else if (x < 2 / d1) {
    return n1 * (x -= 1.5 / d1) * x + 0.75;
  }
  else if (x < 2.5 / d1) {
    return n1 * (x -= 2.25 / d1) * x + 0.9375;
  }
  else {
    return n1 * (x -= 2.625 / d1) * x + 0.984375;
  }
}

static double easeInOutBounce(double x)
{
  return x < 0.5
    ? (1 - easeOutBounce(1 - 2 * x)) / 2
    : (1 + easeOutBounce(2 * x - 1)) / 2;
}

static double ease(double x, Easing easing)
{
  switch (easing) {
  case Easing::InCubic:
    return easeInCubic(x);
  case Easing::InOutBounce:
    return easeInOutBounce(x);
  case Easing::InOutCubic:
    return easeInOutCubic(x);
  case Easing::InOutElastic:
    return easeInOutElastic(x);
  case Easing::InOutSine:
    return easeInOutSine(x);
  case Easing::Linear:
    return x;
  case Easing::OutBounce:
    return easeOutBounce(x);
  case Easing::OutCubic:
    return easeOutCubic(x);
  }

  return x;
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

  double easedFactor = ease(factor, easing);

  q_out = glm::slerp(q0, q1, (float)easedFactor);

  s_out = (1.0f - (float)easedFactor) * s0 + (float)easedFactor * s1;
  t_out = (1.0f - (float)easedFactor) * t0 + (float)easedFactor * t1;

  glm::mat4 rotMat = glm::toMat4(q_out);
  glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), s_out);
  glm::mat4 transMat = glm::translate(glm::mat4(1.0f), t_out);

  out = transMat * rotMat * scaleMat;

  return out;
}

}