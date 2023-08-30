#pragma once

#include <cstddef>

#include "Vertex.h"
#include "PbrMaterialData.h"
#include "MeshId.h"

#include <vector>

#include <vulkan/vulkan.h>

namespace render {

struct Mesh {
  MeshId _id;

  std::vector<Vertex> _vertices;
  std::vector<std::uint32_t> _indices;

  // Offsets into the fat mesh buffer
  std::size_t _vertexOffset;
  std::size_t _indexOffset;

  // BLAS reference
  std::uint64_t _blasRef;

  // These are relative to fat buffer, not necessarily corresponding to verts stored in here
  std::size_t _numVertices;
  std::size_t _numIndices;

  PbrMaterialData _metallic;
  PbrMaterialData _roughness;
  PbrMaterialData _normal;
  PbrMaterialData _emissive;
  PbrMaterialData _albedo;
  PbrMaterialData _metallicRoughness; // i.e. gltf-style, G is roughness and B is metalness
  glm::vec4 _baseColFactor;
  glm::vec3 _emissiveFactor;
};

} 