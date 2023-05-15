#pragma once

namespace render {

struct RenderOptions
{
  bool ssao = true;
  bool fxaa = true;
  bool directionalShadows = false;
  bool raytracedShadows = true;
  bool visualizeBoundingSpheres = false;
};

}