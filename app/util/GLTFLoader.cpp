#include "GLTFLoader.h"

#include "../render/Mesh.h"

#include <limits>
#include <filesystem>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../tinygltf/tiny_gltf.h"

bool GLTFLoader::loadFromFile(
  const std::string& path,
  render::Model& modelOut)
{
  std::filesystem::path p(path);
  auto extension = p.extension().string();

  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err, warn;

  bool ret = false;
  if (extension == ".glb") {
    ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
  }
  else if (extension == ".gltf") {
    ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
  }

  if (!warn.empty()) {
    printf("GLTF warning: %s\n", warn.c_str());
  }

  if (!err.empty()) {
    printf("GLTF ERROR: %s\n", err.c_str());
    return false;
  }

  if (!ret) {
    printf("Failed to parse glTF\n");
    return false;
  }

  // Init min/max
  modelOut._min = glm::vec3(std::numeric_limits<float>::max());
  modelOut._max = glm::vec3(std::numeric_limits<float>::min());

  for (int i = 0; i < model.meshes.size(); ++i) {
    for (int j = 0; j < model.meshes[i].primitives.size(); ++j) {
      auto& primitive = model.meshes[i].primitives[j];
      render::Mesh mesh{};

      // Do indices
      auto& idxAccessor = model.accessors[primitive.indices];
      auto& idxBufferView = model.bufferViews[idxAccessor.bufferView];
      auto& idxBuffer = model.buffers[idxBufferView.buffer];
      auto idxBufferStart = idxBufferView.byteOffset + idxAccessor.byteOffset;

      mesh._indices.resize(idxAccessor.count);

      if (idxAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT) {
        memcpy(mesh._indices.data(), &idxBuffer.data[idxBufferStart], idxAccessor.count * sizeof(uint32_t));
      }
      else if (idxAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT) {
        uint16_t* buf = new uint16_t[idxAccessor.count];
        memcpy(buf, &idxBuffer.data[idxBufferStart], idxAccessor.count * sizeof(uint16_t));

        for (int i = 0; i < idxAccessor.count; ++i) {
          mesh._indices[i] = static_cast<uint32_t>(buf[i]);
        }

        delete[] buf;
      }
      else if (idxAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE) {
        uint8_t* buf = new uint8_t[idxAccessor.count];
        memcpy(buf, &idxBuffer.data[idxBufferStart], idxAccessor.count * sizeof(uint8_t));

        for (int i = 0; i < idxAccessor.count; ++i) {
          mesh._indices[i] = static_cast<uint32_t>(buf[i]);
        }

        delete[] buf;
      }

      // Now copy vertex data
      const float* positionBuffer = nullptr;
      const float* normalsBuffer = nullptr;
      const float* texCoordsBuffer = nullptr;
      const float* colorBuffer = nullptr;
      size_t vertexCount = 0;

      if (primitive.attributes.count("POSITION") > 0) {
        const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("POSITION")->second];
        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
        positionBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
        vertexCount = accessor.count; // Note: According to spec all attributes MUST have same count, so this is ok

        // Check min/max against current in model
        glm::vec3 min{accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]};
        glm::vec3 max{accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]};

        modelOut._min = glm::min(min, modelOut._min);
        modelOut._max = glm::max(max, modelOut._max);
      }
      if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
        const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("NORMAL")->second];
        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
        normalsBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
      }
      if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
        const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
        texCoordsBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
      }
      if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
        const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
        colorBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
      }

      mesh._vertices.resize(vertexCount);

      for (size_t v = 0; v < vertexCount; ++v) {
        render::Vertex vert{};

        vert.pos = {
          positionBuffer[3 * v + 0],
          positionBuffer[3 * v + 1],
          positionBuffer[3 * v + 2]
        };

        if (normalsBuffer) {
          vert.normal = {
            normalsBuffer[3 * v + 0],
            normalsBuffer[3 * v + 1],
            normalsBuffer[3 * v + 2]
          };
        }

        if (texCoordsBuffer) {
          vert.uv = {
            texCoordsBuffer[2 * v + 0],
            texCoordsBuffer[2 * v + 1],
          };
        }

        if (colorBuffer) {
          vert.color = {
            colorBuffer[3 * v + 0],
            colorBuffer[3 * v + 1],
            colorBuffer[3 * v + 2]
          };
        }

        mesh._vertices[v] = std::move(vert);
      }

      // PBR materials
      if (primitive.material >= 0) {
        auto& material = model.materials[primitive.material];

        int baseColIdx = material.pbrMetallicRoughness.baseColorTexture.index;
        if (baseColIdx >= 0) {
          int imageIdx = model.textures[baseColIdx].source;
          auto& image = model.images[imageIdx];

          mesh._albedo._texPath = p.replace_filename(image.uri).string();
          mesh._albedo._data = image.image;
          mesh._albedo._width = image.width;
          mesh._albedo._height = image.height;
        }

        int metRoIdx = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
        if (metRoIdx >= 0) {
          int imageIdx = model.textures[metRoIdx].source;
          auto& image = model.images[imageIdx];

          mesh._metallicRoughness._texPath = p.replace_filename(image.uri).string();
          mesh._metallicRoughness._data = image.image;
          mesh._metallicRoughness._width = image.width;
          mesh._metallicRoughness._height = image.height;
        }
      }

      modelOut._meshes.emplace_back(std::move(mesh));
    }
  }

  printf("Loaded model %s. %zu meshes (min %f %f %f, max %f %f %f)\n",
    path.c_str(), modelOut._meshes.size(),
    modelOut._min.x, modelOut._min.y, modelOut._min.z,
    modelOut._max.x, modelOut._max.y, modelOut._max.z);

  return true;
}
