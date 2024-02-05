#pragma once

#include "../Vertex.h"

namespace render::debug {

struct Triangle
{
  render::Vertex _v0;
  render::Vertex _v1;
  render::Vertex _v2;
};

}