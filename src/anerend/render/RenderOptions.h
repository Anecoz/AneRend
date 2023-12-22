#pragma once

#include <glm/glm.hpp>

namespace render {

struct RenderOptions
{
  bool ssao = true;
  bool fxaa = false;
  bool directionalShadows = false;
  bool pointShadows = true;
  bool raytracedShadows = true;
  bool visualizeBoundingSpheres = false;
  bool raytracingEnabled = true;
  bool ddgiEnabled = true;
  bool multiBounceDdgiEnabled = true;
  bool specularGiEnabled = true;
  bool screenspaceProbes = false;
  bool probesDebug = false;
  bool hack = false;
  float sunIntensity = 5.0;
  float skyIntensity = 1.0;
  float exposure = 1.0;
  glm::vec3 skyColor{ 0.2f, 0.5f, 0.7f };
  float bloomThresh = 1.2f;
  float bloomKnee = 0.1f;
};

}