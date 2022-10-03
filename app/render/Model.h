#pragma once

#include "Vertex.h"
#include "MaterialId.h"

#include <cstdint>
#include <string>
#include <vector>

namespace render {

class Model
{
public:
  Model();
  Model(MaterialID materialID);
  ~Model();

  Model(const Model&);
  Model(Model&&);
  Model& operator=(const Model&);
  Model& operator=(Model&&);

  bool loadFromObj(const std::string& objPath, const std::string& mtlPath);

  explicit operator bool() const;

  std::vector<Vertex> _vertices;
  std::vector<std::uint32_t> _indices;
  MaterialID _materialId;
};
}