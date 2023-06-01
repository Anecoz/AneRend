#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace render {

struct Vertex
{
  glm::vec4 pos;
  glm::vec4 color;
  glm::vec4 normal;
  glm::vec4 tangent;
  glm::vec4 uv;

  bool operator==(const Vertex& other) const {
    return pos == other.pos && color == other.color && uv == other.uv && normal == other.normal;
  }
};

}

namespace std {
  template<> struct hash<render::Vertex> {
    size_t operator()(render::Vertex const& vertex) const {
      return 
        ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
        ((hash<glm::vec3>()(vertex.normal) ^ (hash<glm::vec2>()(vertex.uv) << 1)));
    }
  };
}