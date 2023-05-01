#pragma once

#include "Mesh.h"

#include <cstdint>
#include <string>
#include <vector>

namespace render {

typedef std::uint32_t ModelId;

class Model
{
public:
  Model();
  ~Model();

  Model(const Model&) = delete;
  Model(Model&&);
  Model& operator=(const Model&) = delete;
  Model& operator=(Model&&);

  bool loadFromObj(
    const std::string& objPath,
    const std::string& mtlPath,
    const std::string& metallicPath = "",
    const std::string& roughnessPath = "",
    const std::string& normalPath = "",
    const std::string& albedoPath = "");

  bool loadFromGLTF(const std::string& gltfPath);

  explicit operator bool() const;

  std::vector<Mesh> _meshes;

  std::string _metallicPath;
  std::string _roughnessPath;
  std::string _normalPath;
  std::string _albedoPath;
};
}