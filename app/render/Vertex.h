#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace render {

struct Vertex
{
  bool operator==(const Vertex& other) const {
    return pos == other.pos && color == other.color && uv == other.uv && normal == other.normal;
  }

  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 normal;
  glm::vec2 uv;
};

}

namespace std {
  template<> struct hash<render::Vertex> {
    size_t operator()(render::Vertex const& vertex) const {
      return (
        (hash<glm::vec3>()(vertex.pos) ^
        (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
        (hash<glm::vec2>()(vertex.uv) << 1);
    }
  };
}