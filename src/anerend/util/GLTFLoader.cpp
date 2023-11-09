#include "GLTFLoader.h"

#include "../render/asset/Mesh.h"
#include "../render/animation/Skeleton.h"

#include <limits>
#include <filesystem>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../tinygltf/tiny_gltf.h"

namespace {

glm::mat4 transformFromNode(const tinygltf::Node& node)
{
  glm::mat4 out{1.0f};

  if (!node.matrix.empty()) {
    out = glm::make_mat4(node.matrix.data());
    return out;
  }

  glm::vec3 trans{0.0f, 0.0f, 0.0f};
  glm::vec3 scale{1.0f, 1.0f, 1.0f};
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
        else if(channel._path == render::anim::ChannelPath::Translation) {
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

render::anim::Skeleton constructSkeleton(const std::vector<tinygltf::Node>& nodes, const tinygltf::Skin& skin, const tinygltf::Model& model)
{
  // NOTE: We say that if there is a "skeleton" entry, this will be the root
  //       transform of all joints, else the first joint is the root.
  //       This may not hold at all, the skeleton might have a parent for instance.
  render::anim::Skeleton skeleton{};

  if (skin.skeleton != -1) {
    // There is a root transform
    render::anim::Joint rootJoint{};
    rootJoint._internalId = skin.skeleton;
    rootJoint._localTransform = transformFromNode(nodes[skin.skeleton]);
    rootJoint._originalLocalTransform = rootJoint._localTransform;
    rootJoint._childrenInternalIds = nodes[skin.skeleton].children;
    skeleton._nonJointRoot = true;
    skeleton._joints.emplace_back(std::move(rootJoint));
  }

  // Check if there is a inverseBindMatrices entry
  const float* ibmBufferPtr = nullptr;
  if (skin.inverseBindMatrices != -1) {
    auto& ibmAccessor = model.accessors[skin.inverseBindMatrices];
    auto& ibmBufferView = model.bufferViews[ibmAccessor.bufferView];

    ibmBufferPtr = reinterpret_cast<const float*>(&(model.buffers[ibmBufferView.buffer].data[ibmAccessor.byteOffset + ibmBufferView.byteOffset]));
  }

  // The calculations/construction below are split up for readability...

  // Start by populating joints
  for (int i = 0; i < skin.joints.size(); ++i) {
    auto& jointNode = nodes[skin.joints[i]];

    render::anim::Joint joint{};
    joint._internalId = skin.joints[i];
    joint._localTransform = transformFromNode(jointNode);
    joint._originalLocalTransform = joint._localTransform;
    joint._childrenInternalIds = jointNode.children;

    if (ibmBufferPtr != nullptr) {
      joint._inverseBindMatrix = glm::make_mat4(ibmBufferPtr + 16 * i);
    }

    skeleton._joints.emplace_back(std::move(joint));
  }

  // Set children
  for (auto& joint: skeleton._joints) {

    for (auto childId : joint._childrenInternalIds) {
      for (auto& child : skeleton._joints) {
        if (child._internalId == childId) {
          joint._children.emplace_back(&child);
          break;
        }
      }
    }
  }

  // Set parents
  for (auto& joint : skeleton._joints) {
    for (auto& parent : skeleton._joints) {
      for (auto childIdInParent : parent._childrenInternalIds) {
        if (childIdInParent == joint._internalId) {
          joint._parent = &parent;
          break;
        }
      }
      if (joint._parent != nullptr) {
        // We found a parent and broke inner loop, break
        break;
      }
    }
  }

  skeleton.calcGlobalTransforms();
  return skeleton;
}

}

bool GLTFLoader::loadFromFile(
  const std::string& path,
  render::asset::Model& modelOut,
  std::vector<render::asset::Material>& materialsOut,
  std::vector<int>& materialIndicesOut,
  render::anim::Skeleton& skeletonOut,
  std::vector<render::anim::Animation>& animationsOut)
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
  /*modelOut._min = glm::vec3(std::numeric_limits<float>::max());
  modelOut._max = glm::vec3(std::numeric_limits<float>::min());*/

  if (!model.animations.empty()) {
    animationsOut = constructAnimations(model.animations, model);
  }

  std::vector<int> parsedMaterials;

  for (int i = 0; i < model.meshes.size(); ++i) {
    
    // Find a potential transform
    // TODO: All sorts of transforms, not just translation...
    glm::vec3 trans = {};
    for (auto& node : model.nodes) {
      if (node.mesh == i) {
        if (!node.translation.empty()) {
          trans = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
        }
        if (node.skin != -1) {
          // Construct skeleton for this mesh
          auto skeleton = constructSkeleton(model.nodes, model.skins[node.skin], model);
          skeletonOut = std::move(skeleton);
        }
      }
    }
    
    for (int j = 0; j < model.meshes[i].primitives.size(); ++j) {
      auto& primitive = model.meshes[i].primitives[j];
      render::asset::Mesh mesh{};

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
        glm::vec3 min{accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]};
        glm::vec3 max{accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]};

        //modelOut._min = glm::min(min, modelOut._min);
        //modelOut._max = glm::max(max, modelOut._max);
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

        vert.pos = vert.pos + trans;

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

      modelOut._meshes.emplace_back(std::move(mesh));

      // PBR materials
      if (primitive.material >= 0) {
        // Have we already parsed this material?
        auto it = std::find(parsedMaterials.begin(), parsedMaterials.end(), primitive.material);
        if (it != parsedMaterials.end()) {
          materialIndicesOut.emplace_back(int(it - parsedMaterials.begin()));
        }
        else {
          auto& material = model.materials[primitive.material];
          render::asset::Material mat{};

          // TODO: Copying image data down here is naive... the same texture can be reused multiple times, a cache would be good
          int baseColIdx = material.pbrMetallicRoughness.baseColorTexture.index;
          if (baseColIdx >= 0) {
            int imageIdx = model.textures[baseColIdx].source;
            auto& image = model.images[imageIdx];

            mat._albedoTex.data = image.image;
            mat._albedoTex.isColor = true;
            mat._albedoTex.width = image.width;
            mat._albedoTex.height = image.height;
          }

          int metRoIdx = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
          if (metRoIdx >= 0) {
            int imageIdx = model.textures[metRoIdx].source;
            auto& image = model.images[imageIdx];

            mat._metallicRoughnessTex.data = image.image;
            mat._metallicRoughnessTex.isColor = true;
            mat._metallicRoughnessTex.width = image.width;
            mat._metallicRoughnessTex.height = image.height;
          }

          int normalIdx = material.normalTexture.index;
          if (normalIdx >= 0) {
            int imageIdx = model.textures[normalIdx].source;
            auto& image = model.images[normalIdx];

            mat._normalTex.data = image.image;
            mat._normalTex.isColor = true;
            mat._normalTex.width = image.width;
            mat._normalTex.height = image.height;
          }

          int emissiveIdx = material.emissiveTexture.index;
          if (emissiveIdx >= 0) {
            int imageIdx = model.textures[emissiveIdx].source;
            auto& image = model.images[emissiveIdx];

            mat._emissiveTex.data = image.image;
            mat._emissiveTex.isColor = true;
            mat._emissiveTex.width = image.width;
            mat._emissiveTex.height = image.height;
          }

          mat._emissive = glm::vec4(material.emissiveFactor[0], material.emissiveFactor[1], material.emissiveFactor[2], 1.0f);

          mat._baseColFactor = glm::vec4(
            material.pbrMetallicRoughness.baseColorFactor[0],
            material.pbrMetallicRoughness.baseColorFactor[1],
            material.pbrMetallicRoughness.baseColorFactor[2],
            material.pbrMetallicRoughness.baseColorFactor[3]);

          materialsOut.emplace_back(std::move(mat));
          int materialIndex = (int)(materialsOut.size() - 1);
          materialIndicesOut.emplace_back(materialIndex);

          parsedMaterials.emplace_back(primitive.material);
        }
      }

      if (deleteJoints) delete[] jointsBuffer;
      //modelOut._meshes.emplace_back(std::move(mesh));
    }
  }

  /*printf("Loaded model %s. %zu meshes (min %f %f %f, max %f %f %f)\n",
    path.c_str(), modelOut._meshes.size(),
    modelOut._min.x, modelOut._min.y, modelOut._min.z,
    modelOut._max.x, modelOut._max.y, modelOut._max.z);*/

  printf("Loaded model %s. %zu meshes, %zu materials\n", path.c_str(), modelOut._meshes.size(), materialsOut.size());

  return true;
}
