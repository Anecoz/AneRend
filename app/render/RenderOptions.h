#pragma once

namespace render {

struct RenderOptions
{
  bool ssao = true;
  bool fxaa = true;
  bool directionalShadows = false;
  bool raytracedShadows = true;
  bool visualizeBoundingSpheres = false;
  bool raytracingEnabled = true;
  bool ddgiEnabled = true;
  bool multiBounceDdgiEnabled = true;
  bool specularGiEnabled = true;
  bool probesDebug = false;
  bool hack = false;
};

}