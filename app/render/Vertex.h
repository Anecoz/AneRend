#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace render {

struct Vertex
{
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 normal;
  glm::vec4 tangent;
  glm::vec4 jointWeights;
  glm::vec2 uv;
  float padding = 0.0; // 20 floats (5 * vec4)
  glm::i16vec4 jointIds; // 8 bytes
  float padding1 = 0.0;
  float padding2 = 0.0;

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