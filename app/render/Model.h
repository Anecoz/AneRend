#pragma once

#include "Mesh.h"
#include "animation/Skeleton.h"
#include "animation/Animation.h"

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

  //Model(const Model&);
  Model(Model&&);
  //Model& operator=(const Model&);
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

  // These are in model space (i.e. have to be scaled by any renderable transform before use)
  glm::vec3 _min;
  glm::vec3 _max;

  std::string _metallicPath;
  std::string _roughnessPath;
  std::string _normalPath;
  std::string _albedoPath;

  anim::Skeleton _skeleton;
  std::vector<anim::Animation> _animations;
};
}