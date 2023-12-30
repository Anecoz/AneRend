#pragma once

namespace render::asset { struct Mesh; }

namespace util {

struct TangentGenerator
{
  static void generateTangents(render::asset::Mesh& mesh);
};

}