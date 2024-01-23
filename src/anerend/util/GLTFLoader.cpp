#include "GLTFLoader.h"

#include "../render/asset/Mesh.h"
//#include "../render/animation/Skeleton.h"
#include "TextureHelpers.h"
#include "TangentGenerator.h"

#include <limits>
#include <filesystem>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../tinygltf/tiny_gltf.h"

namespace util {

namespace {

  glm::mat4 transformFromNode(const tinygltf::Node& node)
  {
    glm::mat4 out{ 1.0f };

    if (!node.matrix.empty()) {
      out = glm::make_mat4(node.matrix.data());
      return out;
    }

    glm::vec3 trans{ 0.0f, 0.0f, 0.0f };
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
    glm::quat rotQuat{ 1.0f, 0.0f, 0.0f, 0.0f };

    if (!node.translation.empty()) {
      trans = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
    }
    if (!node.scale.empty()) {
      scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
    }
    if (!node.rotation.empty()) {
      rotQuat = glm::quat((float)node.rotation[3], (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2]);
    }

    glm::mat4 rotMat = glm::toMat4(rotQuat);
    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
    glm::mat4 transMat = glm::translate(glm::mat4(1.0f), trans);

    out = transMat * rotMat * scaleMat;
    return out;
  }

  render::anim::ChannelPath convertPath(const std::string& path)
  {
    if (path == "rotation") {
      return render::anim::ChannelPath::Rotation;
    }
    if (path == "translation") {
      return render::anim::ChannelPath::Translation;
    }
    if (path == "scale") {
      return render::anim::ChannelPath::Scale;
    }

    return render::anim::ChannelPath::Translation;
  }

  std::vector<render::anim::Animation> constructAnimations(const std::vector<tinygltf::Animation>& anims, const tinygltf::Model& model)
  {
    std::vector<render::anim::Animation> out;
    out.reserve(anims.size());

    for (auto& gltfAnim : anims) {
      render::anim::Animation animation{};
      animation._name = gltfAnim.name;

      for (auto& gltfChannel : gltfAnim.channels) {
        render::anim::Channel channel{};
        channel._internalId = gltfChannel.target_node;
        channel._path = convertPath(gltfChannel.target_path);

        // Access input and output buffers
        auto& sampler = gltfAnim.samplers[gltfChannel.sampler];

        auto& iaccessor = model.accessors[sampler.input];
        auto& ibufferView = model.bufferViews[iaccessor.bufferView];
        auto times = reinterpret_cast<const float*>(&(model.buffers[ibufferView.buffer].data[iaccessor.byteOffset + ibufferView.byteOffset]));

        auto& oaccessor = model.accessors[sampler.output];
        auto& obufferView = model.bufferViews[oaccessor.bufferView];
        auto poses = reinterpret_cast<const float*>(&(model.buffers[obufferView.buffer].data[oaccessor.byteOffset + obufferView.byteOffset]));

        for (std::size_t i = 0; i < iaccessor.count; ++i) {
          channel._inputTimes.emplace_back(times[i]);

          glm::vec4 vec{ 0.0f };
          if (channel._path == render::anim::ChannelPath::Rotation) {
            vec = { poses[i * 4 + 3], poses[i * 4 + 0], poses[i * 4 + 1], poses[i * 4 + 2] };
          }
          else if (channel._path == render::anim::ChannelPath::Translation) {
            vec = { poses[i * 3 + 0], poses[i * 3 + 1], poses[i * 3 + 2], 0.0f };
          }
          else if (channel._path == render::anim::ChannelPath::Scale) {
            vec = { poses[i * 3 + 0], poses[i * 3 + 1], poses[i * 3 + 2], 0.0f };
          }

          channel._outputs.emplace_back(std::move(vec));
        }

        animation._channels.emplace_back(std::move(channel));
      }

      out.emplace_back(std::move(animation));
    }

    return out;
  }
}

bool GLTFLoader::loadFromFile(
  const std::string& path,
  std::vector<render::asset::Prefab>& prefabsOut,
  std::vector<render::asset::Model>& modelsOut,
  std::vector<render::asset::Texture>& texturesOut,
  std::vector<render::asset::Material>& materialsOut,
  std::vector<render::anim::Animation>& animationsOut)
{
  std::filesystem::path p(path);
  auto extension = p.extension().string();
  auto baseName = p.stem().string();

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

  if (!model.animations.empty()) {
    animationsOut = constructAnimations(model.animations, model);
  }

  std::vector<int> parsedMaterials;
  std::unordered_map<int, util::Uuid> parsedModels; // <node, modelsOut id>
  std::unordered_map<int, std::vector<int>> parsedMaterialIndices;
  std::unordered_map<int, util::Uuid> parsedTextures;
  std::unordered_map<int, std::size_t> prefabMap; // <node, prefabsOut idx>
  std::unordered_map<int, component::Skeleton> parsedSkeletons;
  std::unordered_map<int, util::Uuid> nodeMap;

  std::size_t numTotalMeshes = 0;
  std::size_t numTotalVerts = 0;

  // Go through all nodes
  for (int i = 0; i < model.nodes.size(); ++i) {
    auto& node = model.nodes[i];

    // Is there a transform to apply?
    glm::mat4 m = transformFromNode(node);

    if (node.mesh != -1) {

      util::Uuid assetModelId;
      std::vector<int> materialIndices;

      if (parsedModels.find(node.mesh) == parsedModels.end()) {

        render::asset::Model assetModel{};
        assetModel._name = baseName;

        if (!node.name.empty()) {
          assetModel._name += "_" + node.name;
        }

        // Parse all primitives as separate asset::Mesh        
        for (int j = 0; j < model.meshes[node.mesh].primitives.size(); ++j) {
          auto& primitive = model.meshes[node.mesh].primitives[j];
          render::asset::Mesh mesh{};

          bool tangentsSet = false;

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
          const float* tangentBuffer = nullptr;
          const float* colorBuffer = nullptr;
          std::int16_t* jointsBuffer = nullptr;
          const float* weightsBuffer = nullptr;
          bool deleteJoints = false;
          size_t vertexCount = 0;

          if (primitive.attributes.count("POSITION") > 0) {
            const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("POSITION")->second];
            const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
            positionBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            vertexCount = accessor.count; // Note: According to spec all attributes MUST have same count, so this is ok

            // Check min/max against current in model
            glm::vec3 min{ accessor.minValues[0], accessor.minValues[1], accessor.minValues[2] };
            glm::vec3 max{ accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2] };

            //min = glm::vec3(m * glm::vec4(min, 1.0f));
            //max = glm::vec3(m * glm::vec4(max, 1.0f));

            mesh._minPos = min;
            mesh._maxPos = max;
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
          if (primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
            const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("TANGENT")->second];
            const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
            tangentBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            tangentsSet = true;
          }
          if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
            const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
            const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
            auto& buffer = model.buffers[view.buffer];
            auto bufferStart = view.byteOffset + accessor.byteOffset;

            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
              if (view.byteStride == 0) {
                uint8_t* buf = new uint8_t[accessor.count * 4];
                memcpy(buf, &buffer.data[bufferStart], accessor.count * 4 * sizeof(uint8_t));

                jointsBuffer = new std::int16_t[accessor.count * 4];
                for (int i = 0; i < accessor.count * 4; ++i) {
                  jointsBuffer[i] = static_cast<int16_t>(buf[i]);
                }

                deleteJoints = true;
                delete[] buf;
              }
              else {
                printf("UNHANDLED JOINTS BYTE STRIDE!\n");
              }
            }
            else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
              if (view.byteStride == 2) {
                jointsBuffer = reinterpret_cast<std::int16_t*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
              }
              else {
                jointsBuffer = new std::int16_t[accessor.count * 4];
                auto* bufDat = buffer.data.data() + accessor.byteOffset + view.byteOffset;
                for (int i = 0; i < accessor.count; ++i) {
                  jointsBuffer[i * 4 + 0] = *reinterpret_cast<std::int16_t*>(bufDat + view.byteStride * i + 0 * sizeof(std::int16_t));
                  jointsBuffer[i * 4 + 1] = *reinterpret_cast<std::int16_t*>(bufDat + view.byteStride * i + 1 * sizeof(std::int16_t));
                  jointsBuffer[i * 4 + 2] = *reinterpret_cast<std::int16_t*>(bufDat + view.byteStride * i + 2 * sizeof(std::int16_t));
                  jointsBuffer[i * 4 + 3] = *reinterpret_cast<std::int16_t*>(bufDat + view.byteStride * i + 3 * sizeof(std::int16_t));
                }

                deleteJoints = true;
              }
            }
          }
          if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
            const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
            const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
            weightsBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
          }

          mesh._vertices.resize(vertexCount);

          for (size_t v = 0; v < vertexCount; ++v) {
            render::Vertex vert{};

            vert.pos = {
              positionBuffer[3 * v + 0],
              positionBuffer[3 * v + 1],
              positionBuffer[3 * v + 2]
            };

            //vert.pos = glm::vec3(m * glm::vec4(vert.pos, 1.0f));

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
                texCoordsBuffer[2 * v + 1]
              };
            }

            if (colorBuffer) {
              vert.color = {
                colorBuffer[3 * v + 0],
                colorBuffer[3 * v + 1],
                colorBuffer[3 * v + 2]
              };
            }
            else {
              vert.color = { 1.0f, 1.0f, 1.0f };
            }

            if (tangentBuffer) {
              vert.tangent = {
                tangentBuffer[4 * v + 0],
                tangentBuffer[4 * v + 1],
                tangentBuffer[4 * v + 2],
                tangentBuffer[4 * v + 3],
              };
            }

            if (jointsBuffer) {
              vert.jointIds = {
                jointsBuffer[4 * v + 0],
                jointsBuffer[4 * v + 1],
                jointsBuffer[4 * v + 2],
                jointsBuffer[4 * v + 3]
              };
            }
            else {
              vert.jointIds = {
                -1, -1, -1, -1
              };
            }

            if (weightsBuffer) {
              vert.jointWeights = {
                weightsBuffer[4 * v + 0],
                weightsBuffer[4 * v + 1],
                weightsBuffer[4 * v + 2],
                weightsBuffer[4 * v + 3]
              };
            }

            mesh._vertices[v] = std::move(vert);
          }

          numTotalMeshes++;
          numTotalVerts += vertexCount;
          assetModel._meshes.emplace_back(std::move(mesh));

          // PBR materials
          if (primitive.material >= 0) {
            // Have we already parsed this material?
            auto it = std::find(parsedMaterials.begin(), parsedMaterials.end(), primitive.material);
            if (it != parsedMaterials.end()) {
              materialIndices.emplace_back(int(it - parsedMaterials.begin()));
            }
            else {
              auto& material = model.materials[primitive.material];
              render::asset::Material mat{};
              mat._name = material.name;
              mat._roughnessFactor = (float)material.pbrMetallicRoughness.roughnessFactor;
              mat._metallicFactor = (float)material.pbrMetallicRoughness.metallicFactor;

              int baseColIdx = material.pbrMetallicRoughness.baseColorTexture.index;
              if (baseColIdx >= 0) {
                int imageIdx = model.textures[baseColIdx].source;

                // has this texture been parsed already?
                if (parsedTextures.find(imageIdx) != parsedTextures.end()) {
                  mat._albedoTex = parsedTextures[imageIdx];
                }
                else {
                  auto& image = model.images[imageIdx];
                  render::asset::Texture tex{};
                  mat._albedoTex = tex._id;

                  auto w = image.width;
                  auto h = image.height;
                  tex._data.emplace_back(std::move(image.image));
                  tex._format = render::asset::Texture::Format::RGBA8_SRGB;
                  tex._width = image.width;
                  tex._height = image.height;
                  parsedTextures[imageIdx] = tex._id;

                  texturesOut.emplace_back(std::move(tex));
                }
              }

              int metRoIdx = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
              if (metRoIdx >= 0) {
                int imageIdx = model.textures[metRoIdx].source;

                // has this texture been parsed already?
                if (parsedTextures.find(imageIdx) != parsedTextures.end()) {
                  mat._metallicRoughnessTex = parsedTextures[imageIdx];
                }
                else {
                  auto& image = model.images[imageIdx];
                  render::asset::Texture tex{};
                  mat._metallicRoughnessTex = tex._id;

                  auto w = image.width;
                  auto h = image.height;
                  tex._data.emplace_back(TextureHelpers::convertRGBA8ToRG8(std::move(image.image)));
                  tex._format = render::asset::Texture::Format::RG8_UNORM;
                  tex._width = w;
                  tex._height = h;
                  parsedTextures[imageIdx] = tex._id;

                  texturesOut.emplace_back(std::move(tex));
                }
              }

              int normalIdx = material.normalTexture.index;
              if (normalIdx >= 0) {
                int imageIdx = model.textures[normalIdx].source;

                // has this texture been parsed already?
                if (parsedTextures.find(imageIdx) != parsedTextures.end()) {
                  mat._normalTex = parsedTextures[imageIdx];
                }
                else {
                  auto& image = model.images[imageIdx];
                  render::asset::Texture tex{};
                  mat._normalTex = tex._id;

                  auto w = image.width;
                  auto h = image.height;
                  tex._data.emplace_back(std::move(image.image));
                  tex._format = render::asset::Texture::Format::RGBA8_UNORM;
                  tex._width = w;
                  tex._height = h;
                  parsedTextures[imageIdx] = tex._id;

                  // Generate tangents if not present
                  if (!tangentsSet) {
                    TangentGenerator::generateTangents(assetModel._meshes.back());
                  }

                  texturesOut.emplace_back(std::move(tex));
                }
              }

              int emissiveIdx = material.emissiveTexture.index;
              if (emissiveIdx >= 0) {
                int imageIdx = model.textures[emissiveIdx].source;

                // has this texture been parsed already?
                if (parsedTextures.find(imageIdx) != parsedTextures.end()) {
                  mat._emissiveTex = parsedTextures[imageIdx];
                }
                else {
                  auto& image = model.images[imageIdx];
                  render::asset::Texture tex{};
                  mat._emissiveTex = tex._id;

                  auto w = image.width;
                  auto h = image.height;
                  tex._data.emplace_back(std::move(image.image));
                  tex._format = render::asset::Texture::Format::RGBA8_SRGB;
                  tex._width = w;
                  tex._height = h;
                  parsedTextures[imageIdx] = tex._id;

                  texturesOut.emplace_back(std::move(tex));
                }
              }

              mat._emissive = glm::vec4(material.emissiveFactor[0], material.emissiveFactor[1], material.emissiveFactor[2], 1.0f);

              mat._baseColFactor = glm::vec4(
                material.pbrMetallicRoughness.baseColorFactor[0],
                material.pbrMetallicRoughness.baseColorFactor[1],
                material.pbrMetallicRoughness.baseColorFactor[2],
                material.pbrMetallicRoughness.baseColorFactor[3]);

              materialsOut.emplace_back(std::move(mat));
              int materialIndex = (int)(materialsOut.size() - 1);
              materialIndices.emplace_back(materialIndex);

              parsedMaterials.emplace_back(primitive.material);
            }
          }

          if (deleteJoints) delete[] jointsBuffer;
        }
        
        parsedModels[node.mesh] = assetModel._id;
        parsedMaterialIndices[node.mesh] = materialIndices;
        assetModelId = assetModel._id;
        modelsOut.emplace_back(std::move(assetModel));
      }
      else {
        assetModelId = parsedModels[node.mesh];
        materialIndices = parsedMaterialIndices[node.mesh];
      }

      render::asset::Prefab prefab{};
      prefab._name = node.name;
      prefab._comps._trans = component::Transform{ m, m };

      component::Renderable rendComp{};
      rendComp._model = assetModelId;
      //rendComp._skeleton = skeleton._id ? skeleton._id : util::Uuid();
      rendComp._name = node.name;
      rendComp._boundingSphere = glm::vec4(0.0f, 0.0f, 0.0f, 30.0f);

      for (auto& idx : materialIndices) {
        rendComp._materials.push_back(materialsOut[idx]._id);
      }

      // Is there a skeleton to parse?
      if (node.skin != -1) {
        // Note: Not caring about the "skeleton" attribute atm, it's not needed to update the hierarchy
        if (parsedSkeletons.find(node.skin) == parsedSkeletons.end()) {
          component::Skeleton skelComp;
          skelComp._name = model.skins[node.skin].name;

          // Check if there is a inverseBindMatrices entry
          const float* ibmBufferPtr = nullptr;
          if (model.skins[node.skin].inverseBindMatrices != -1) {
            auto& ibmAccessor = model.accessors[model.skins[node.skin].inverseBindMatrices];
            auto& ibmBufferView = model.bufferViews[ibmAccessor.bufferView];

            ibmBufferPtr = reinterpret_cast<const float*>(&(model.buffers[ibmBufferView.buffer].data[ibmAccessor.byteOffset + ibmBufferView.byteOffset]));
          }

          // The uuids will be set at the end
          for (std::size_t j = 0; j < model.skins[node.skin].joints.size(); ++j) {
            component::Skeleton::JointRef jr;
            jr._internalId = model.skins[node.skin].joints[j];
            // Note that this order here is extremely important!
            // The vertex attributes rely on this order for their "jointIds" to point correctly.
            if (ibmBufferPtr != nullptr) {
              jr._inverseBindMatrix = glm::make_mat4(ibmBufferPtr + 16 * j);
            }
            skelComp._jointRefs.emplace_back(std::move(jr));
          }

          parsedSkeletons[node.skin] = skelComp;
          prefab._comps._skeleton = skelComp;
        }
        else {
          prefab._comps._skeleton = parsedSkeletons[node.skin];
        }

        // Add animator to this node. Animations will be pre-filled at the end.
        component::Animator animator;
        prefab._comps._animator = std::move(animator);
      }

      prefab._comps._rend = std::move(rendComp);

      prefabsOut.emplace_back(std::move(prefab));
      prefabMap[i] = prefabsOut.size() - 1;

      /*if (skeleton._id) {
        skeletonsOut.emplace_back(std::move(skeleton));
      }*/
    }
    else {
      // Just a transform, but still a node
      render::asset::Prefab prefab{};
      prefab._name = baseName;

      if (!node.name.empty()) {
        prefab._name += "_" + node.name;
      }

      prefab._comps._trans = component::Transform{ m, m };

      prefabsOut.emplace_back(std::move(prefab));
      prefabMap[i] = prefabsOut.size() - 1;
    }

    nodeMap[i] = prefabsOut.back()._id;
  }

  // Set children after all nodes have been parsed
  for (auto& pair : prefabMap) {
    auto& node = model.nodes[pair.first];
    auto& prefab = prefabsOut[pair.second];

    for (int childNode : node.children) {
      auto it = prefabMap.find(childNode);

      if (it != prefabMap.end()) {
        auto& childPrefab = prefabsOut[prefabMap[childNode]];
        childPrefab._parent = prefab._id;
        prefab._children.emplace_back(childPrefab._id);
      }
    }
  }

  // Set internal uuids of skeleton components now that everything is parsed
  for (auto& prefab : prefabsOut) {
    if (prefab._comps._skeleton) {
      for (auto& ref : prefab._comps._skeleton.value()._jointRefs) {
        ref._node = nodeMap[ref._internalId];
      }
    }

    // Pre-fill animation ids in all animators
    if (prefab._comps._animator) {
      auto& anim = prefab._comps._animator.value();
      for (auto& animation : animationsOut) {
        anim._animations.emplace_back(animation._id);
        anim._currentAnimation = animation._id;
      }
    }
  }

  // Check the scene, to get a top level scene::Node to output
  if (!model.scenes.empty()) {
    glm::mat4 m{ 1.0f };

    auto& scene = model.scenes[0];
    render::asset::Prefab prefab{};
    prefab._name = baseName + "_" + (scene.name.empty() ? "top" : scene.name);

    prefab._comps._trans = component::Transform{ m, m };

    // Set children
    for (int childNode : scene.nodes) {
      auto it = prefabMap.find(childNode);

      if (it != prefabMap.end()) {
        auto& childPrefab = prefabsOut[prefabMap[childNode]];
        childPrefab._parent = prefab._id;
        prefab._children.emplace_back(childPrefab._id);
      }
    }

    prefabsOut.emplace_back(std::move(prefab));
  }

  printf("Loaded GLTF %s. \n\t%zu models containing %zu meshes \n\t%zu verts \n\t%zu skeletons \n\t%zu animations \n\t%zu materials \n\t%zu textures\n",
    path.c_str(), modelsOut.size(), numTotalMeshes, numTotalVerts, parsedSkeletons.size(), animationsOut.size(), materialsOut.size(), texturesOut.size());

  return true;
}

}