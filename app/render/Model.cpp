#include "Model.h"

#include "../util/OBJLoader.h"
#include "../util/GLTFLoader.h"
#include "Vertex.h"

#include <limits>

namespace render
{

Model::Model()
{}

Model::~Model()
{}

/*Model::Model(const Model& rhs)
{
  _meshes = rhs._meshes;
  _min = rhs._min;
  _max = rhs._max;
  _metallicPath = rhs._metallicPath;
  _roughnessPath = rhs._roughnessPath;
  _normalPath = rhs._normalPath;
  _albedoPath = rhs._albedoPath;
}*/

Model::Model(Model&& rhs)
{
  std::swap(_meshes, rhs._meshes);
}

/*Model& Model::operator=(const Model& rhs)
{
  if (this != &rhs) {
    _meshes = rhs._meshes;
    _min = rhs._min;
    _max = rhs._max;
    _metallicPath = rhs._metallicPath;
    _roughnessPath = rhs._roughnessPath;
    _normalPath = rhs._normalPath;
    _albedoPath = rhs._albedoPath;
  }
  return *this;
}*/

Model& Model::operator=(Model&& rhs)
{
  if (this != &rhs) {
    std::swap(_meshes, rhs._meshes);
  }
  return *this;
}

Model::operator bool() const
{
  return !_meshes.empty();
}

bool Model::loadFromObj(
  const std::string& objPath,
  const std::string& mtlPath,
  const std::string& metallicPath,
  const std::string& roughnessPath,
  const std::string& normalPath,
  const std::string& albedoPath)
{
  // TODO: Obj only supports 1 mesh (meaning only 1 set of material textures) atm

  Mesh mesh{};

  if (!OBJLoader::loadFromFile(objPath, mtlPath, mesh._vertices, mesh._indices)) {
    printf("Could not load OBJ from %s and %s\n", objPath.c_str(), mtlPath.c_str());
    return false;
  }

  if (mesh._indices.empty()) {
    printf("No indices in obj file!\n");
    return false;
  }

  mesh._metallic._texPath = metallicPath;
  mesh._roughness._texPath = roughnessPath;
  mesh._normal._texPath = normalPath;
  mesh._albedo._texPath = albedoPath;

  _meshes.emplace_back(std::move(mesh));

  return true;
}

bool Model::loadFromGLTF(const std::string& gltfPath)
{
  Model& model = *this;
  return GLTFLoader::loadFromFile(gltfPath, model);
}

}