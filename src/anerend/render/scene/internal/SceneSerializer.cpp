#include "SceneSerializer.h"

#include "../../../component/Components.h"
#include "../Scene.h"

#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>
#include <bitsery/traits/string.h>
#include <bitsery/ext/std_optional.h>

#include <cstdint>
#include <fstream>

namespace {

// Intermediate representation for nodes
struct IntermediateNode
{
  render::scene::Node _node;
  component::PotentialComponents _comps;
};

}

namespace bitsery
{

template <typename S>
void serialize(S& s, util::Uuid& uuid)
{
  auto bytes = uuid.bytes();
  s.container1b(bytes, 16);

  std::array<std::uint8_t, 16> data;
  std::copy_n(std::make_move_iterator(bytes.begin()), 16, data.begin());
  uuid = util::Uuid(std::move(data));
}

template <typename S>
void serialize(S& s, glm::mat4& m)
{
  s.value4b(m[0][0]);
  s.value4b(m[0][1]);
  s.value4b(m[0][2]);
  s.value4b(m[0][3]);

  s.value4b(m[1][0]);
  s.value4b(m[1][1]);
  s.value4b(m[1][2]);
  s.value4b(m[1][3]);

  s.value4b(m[2][0]);
  s.value4b(m[2][1]);
  s.value4b(m[2][2]);
  s.value4b(m[2][3]);

  s.value4b(m[3][0]);
  s.value4b(m[3][1]);
  s.value4b(m[3][2]);
  s.value4b(m[3][3]);
}

template <typename S>
void serialize(S& s, glm::quat& v)
{
  s.value4b(v.x);
  s.value4b(v.y);
  s.value4b(v.z);
  s.value4b(v.w);
}

template <typename S>
void serialize(S& s, glm::vec4& v)
{
  s.value4b(v.x);
  s.value4b(v.y);
  s.value4b(v.z);
  s.value4b(v.w);
}

template <typename S>
void serialize(S& s, glm::vec3& v)
{
  s.value4b(v.x);
  s.value4b(v.y);
  s.value4b(v.z);
}

template <typename S>
void serialize(S& s, glm::vec2& v)
{
  s.value4b(v.x);
  s.value4b(v.y);
}

template <typename S>
void serialize(S& s, glm::i16vec4& v)
{
  s.value2b(v.x);
  s.value2b(v.y);
  s.value2b(v.z);
  s.value2b(v.w);
}

template <typename S>
void serialize(S& s, glm::ivec2& v)
{
  s.value4b(v.x);
  s.value4b(v.y);
}

template <typename S>
void serialize(S& s, render::Vertex& v)
{
  s.object(v.pos);
  s.object(v.color);
  s.object(v.normal);
  s.object(v.tangent);
  s.object(v.jointWeights);
  s.object(v.uv);
  s.object(v.jointIds);
}

template <typename S>
void serialize(S& s, component::Transform& p)
{
  s.object(p._globalTransform);
  s.object(p._localTransform);
}

template <typename S>
void serialize(S& s, component::Animator& p)
{
  s.value1b(p._state);
  s.container(p._animations, 100);
  s.object(p._currentAnimation);
  s.value4b(p._playbackMultiplier);
}

template <typename S>
void serialize(S& s, component::Skeleton::JointRef& p)
{
  s.value4b(p._internalId);
  s.object(p._inverseBindMatrix);
  s.object(p._node);
}

template <typename S>
void serialize(S& s, component::Skeleton& p)
{
  s.text1b(p._name, 100);
  s.container(p._jointRefs, 100);
}

template <typename S>
void serialize(S& s, component::PotentialComponents& p)
{
  s.object(p._trans);
  s.ext(p._rend, bitsery::ext::StdOptional{});
  s.ext(p._light, bitsery::ext::StdOptional{});
  s.ext(p._animator, bitsery::ext::StdOptional{});
  s.ext(p._skeleton, bitsery::ext::StdOptional{});
}

template <typename S>
void serialize(S& s, IntermediateNode& p)
{
  s.object(p._node._id);
  s.text1b(p._node._name, 100);
  s.object(p._node._parent);
  s.container(p._node._children, 1000);
  s.object(p._comps);
}

template <typename S>
void serialize(S& s, std::vector<IntermediateNode>& v)
{
  s.container(v, 20000);
}

template <typename S>
void serialize(S& s, render::asset::Prefab& p)
{
  s.object(p._id);
  s.text1b(p._name, 100);
  s.object(p._comps);
  s.object(p._parent);
  s.container(p._children, 1000);
}

template <typename S>
void serialize(S& s, std::vector<render::asset::Prefab>& v)
{
  s.container(v, 2500);
}

template <typename S>

void serialize(S& s, render::asset::Mesh& m)
{
  s.object(m._id);
  s.container(m._vertices, 500000);
  s.container4b(m._indices, 500000);
  s.object(m._minPos);
  s.object(m._maxPos);
}

template <typename S>
void serialize(S& s, render::asset::Model& m)
{
  s.object(m._id);
  s.text1b(m._name, 100);
  s.container(m._meshes, 2500);
}

template <typename S>
void serialize(S& s, std::vector<render::asset::Model>& v)
{
  s.container(v, 2500);
}

template <typename S>
void serialize(S& s, render::asset::Material& m)
{
  s.object(m._id);
  s.text1b(m._name, 100);
  s.object(m._baseColFactor);
  s.value4b(m._roughnessFactor);
  s.value4b(m._metallicFactor);
  s.object(m._emissive);
  s.object(m._metallicRoughnessTex);
  s.object(m._albedoTex);
  s.object(m._normalTex);
  s.object(m._emissiveTex); 
}

template <typename S>
void serialize(S& s, std::vector<render::asset::Material>& m)
{
  s.container(m, 1000);
}

template <typename S>
void serialize(S& s, render::asset::Texture& t)
{
  s.object(t._id);
  s.value1b(t._format);
  s.value4b(t._width);
  s.value4b(t._height);
  s.value4b(t._numMips);
  // For deserialisation
  if (t._data.empty()) {
    for (unsigned i = 0; i < t._numMips; ++i) {
      t._data.emplace_back();
    }
  }
  for (unsigned i = 0; i < t._numMips; ++i) {
    s.container1b(t._data[i], 100'000'000); // 100MB
  }
}

template <typename S>
void serialize(S& s, std::vector<render::asset::Texture>& t)
{
  s.container(t, 500);
}

template <typename S>
void serialize(S& s, render::anim::Channel& c)
{
  s.value4b(c._internalId);
  s.value1b(c._path);
  s.container4b(c._inputTimes, 10000);
  s.container(c._outputs, 10000);
}

template <typename S>
void serialize(S& s, render::anim::Animation& a)
{
  s.object(a._id);
  s.text1b(a._name, 100);
  s.container(a._channels, 100);
}

template <typename S>
void serialize(S& s, std::vector<render::anim::Animation>& a)
{
  s.container(a, 100);
}

template <typename S>
void serialize(S& s, component::Renderable& r)
{
  //s.object(r._id);
  s.text1b(r._name, 100);
  s.object(r._model);
  //s.object(r._skeleton);
  s.container(r._materials, 2048);
  s.object(r._tint);
  s.object(r._boundingSphere);
  s.value1b(r._visible);
}

template <typename S>
void serialize(S& s, std::vector<component::Renderable>& r)
{
  s.container(r, 100'000);
}

template <typename S>
void serialize(S& s, component::Light& l)
{
  //s.object(l._id);
  s.text1b(l._name, 100);
  s.object(l._color);
  s.value4b(l._range);
  s.value1b(l._enabled);
  s.value1b(l._shadowCaster);
}

template <typename S>
void serialize(S& s, std::vector<component::Light>& v)
{
  s.container(v, 1024);
}

template <typename S>
void serialize(S& s, render::scene::TileIndex& t)
{
  s.object(t._idx);
  s.value1b(t._initialized);
}

template <typename S>
void serialize(S& s, render::scene::Scene::TileInfo& t)
{
  s.object(t._idx);
  s.object(t._ddgiAtlas);
}

template <typename S>
void serialize(S& s, std::vector<render::scene::Scene::TileInfo>& t)
{
  s.container(t, 1024);
}

template <typename S>
void serialize(S& s, render::asset::CameraKeyframe& t)
{
  s.value1b(t._easing);
  s.value4b(t._time);
  s.object(t._pos);
  s.object(t._orientation);
  s.object(t._ypr);
}

template <typename S>
void serialize(S& s, render::asset::NodeKeyframe& t)
{
  s.value1b(t._easing);
  s.value4b(t._time);
  s.object(t._id);
  s.object(t._comps);
}

template <typename S>
void serialize(S& s, std::vector<render::asset::NodeKeyframe>& t)
{
  s.container(t, 500);
}

template <typename S>
void serialize(S& s, render::asset::MaterialKeyframe& t)
{
  s.value4b(t._time);
  s.value1b(t._easing);
  s.object(t._id);
  s.object(t._emissive);
  s.object(t._baseColFactor);
}

template <typename S>
void serialize(S& s, std::vector<render::asset::MaterialKeyframe>& t)
{
  s.container(t, 500);
}

template <typename S>
void serialize(S& s, render::asset::Cinematic& t)
{
  s.object(t._id);
  s.text1b(t._name, 100);
  s.value4b(t._maxTime);
  s.container(t._camKeyframes, 100);
  s.container(t._nodeKeyframes, 100);
  s.container(t._materialKeyframes, 100);
}

template <typename S>
void serialize(S& s, std::vector<render::asset::Cinematic>& t)
{
  s.container(t, 50);
}

}

static std::uint8_t g_CurrVersion = 1;

namespace render::scene::internal {

namespace {

template <typename T>
std::vector<std::uint8_t> serializeVector(const T& object) {
  std::vector<std::uint8_t> serializedData;
  auto byteSize = (uint32_t)bitsery::quickSerialization<bitsery::OutputBufferAdapter<std::vector<std::uint8_t>>>(serializedData, object);
  serializedData.resize(byteSize);
  return serializedData;
}

class InternalSerializer
{
public:
  // Add a type to be serialized
  template <typename T>
  void add(const T object) {
    auto serializedData = serializeVector(object);
    _serializedData.insert(_serializedData.end(), serializedData.begin(), serializedData.end());
    _indices.push_back((std::uint32_t)_serializedData.size());
  }

  // Serialize the added data and write to file
  void serializeToFile(const std::string& filePath) {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
      printf("Could not open file for serialization!\n");
      return;
    }

    std::uint32_t headerSize = (std::uint32_t)_indices.size() * sizeof(std::uint32_t) + (std::uint32_t)sizeof(g_CurrVersion);
    file.write(reinterpret_cast<const char*>(&g_CurrVersion), sizeof(g_CurrVersion));

    // This will be the first data index
    file.write(reinterpret_cast<const char*>(&headerSize), sizeof(headerSize));

    // All other indices
    for (std::size_t i = 0; i < _indices.size() - 1; ++i) {
      auto index = _indices[i];
      index += headerSize;
      file.write(reinterpret_cast<const char*>(&index), sizeof(index));
    }

    // All data in one go
    file.write(reinterpret_cast<const char*>(_serializedData.data()), _serializedData.size());
    file.close();
  }

private:
  std::vector<std::uint8_t> _serializedData;
  std::vector<std::uint32_t> _indices;
};

std::uint32_t headerSize()
{
  return
    1 + // ver
    4 + // prefab idx
    4 + // texture idx
    4 + // model idx
    4 + // mat idx
    4 + // anim idx
    4 + // tile info idx
    4 + // nodes idx
    4;  // cinematic idx
}

}

SceneSerializer::~SceneSerializer()
{
  if (_serialisationThread.joinable()) {
    _serialisationThread.join();
  }

  if (_deserialisationThread.joinable()) {
    _deserialisationThread.join();
  }
}

void SceneSerializer::serialize(Scene& scene, const std::filesystem::path& path)
{
  // Copy data from scene and then launch a thread to do the serialisation

  if (_serialisationThread.joinable()) {
    _serialisationThread.join();
  }

  _serialisationThread = std::thread(
    [
      &scene,
      path,
      version = g_CurrVersion
    ]() {

      // Essentially we try to pack all the assets as close to binary compatible w/ c++ mem layout as possible,
      // to make deserialization easier

      /*
        File structure (version 1):

        -- header--
        1 byte  version         (uint8_t)
        4 bytes prefab idx      (uint32_t)
        4 bytes tex idx         (uint32_t)
        4 bytes model idx       (uint32_t)
        4 bytes material idx    (uint32_t)
        4 bytes animation idx   (uint32_t)
        4 bytes tileInfo   idx  (uint32_t)
        4 bytes nodes      idx  (uint32_t)
        4 bytes cinematic  idx  (uint32_t)

        -- prefabs --

        -- textures --

        -- models --

        -- materials --

        -- animations --

        -- tile infos --

        -- nodes --

        -- cinematics --

        EOF
      */

      // Copy assets. scene is a reference here in order to avoid copying on calling thread,
      // but there currently is no real guarantee that scene is alive here.
      // TODO: Should probably add a mechanism to make sure that scene stays alive for this duration.

      // Translate all nodes into intermediate representation
      std::vector<IntermediateNode> imNodes;
      for (auto& node : scene._nodeVec) {
        IntermediateNode imn{};
        imn._node._id = node._id;
        imn._node._name = node._name;
        imn._node._parent = node._parent;
        imn._node._children = node._children;

        // Components
        imn._comps._trans = scene.registry().getComponent<component::Transform>(node._id);

        if (scene.registry().hasComponent<component::Renderable>(node._id)) {
          imn._comps._rend = scene.registry().getComponent<component::Renderable>(node._id);
        }
        if (scene.registry().hasComponent<component::Light>(node._id)) {
          imn._comps._light = scene.registry().getComponent<component::Light>(node._id);
        }
        if (scene.registry().hasComponent<component::Skeleton>(node._id)) {
          imn._comps._skeleton = scene.registry().getComponent<component::Skeleton>(node._id);
        }
        if (scene.registry().hasComponent<component::Animator>(node._id)) {
          imn._comps._animator = scene.registry().getComponent<component::Animator>(node._id);
        }

        imNodes.emplace_back(std::move(imn));
      }

      InternalSerializer ser;
      ser.add(scene._prefabs);
      ser.add(scene._textures);
      ser.add(scene._models);
      ser.add(scene._materials);
      ser.add(scene._animations);
      ser.add(scene._tileInfos);
      ser.add(imNodes);
      ser.add(scene._cinematics);

      ser.serializeToFile(path.string());

      printf("Scene serialisation done!\n");
    });
}

namespace {

template <typename T>
bool desHelper(std::ifstream& file, uint32_t idx, std::vector<uint8_t>& preSizedVec, T& t)
{
  file.seekg(idx);
  file.read((char*)preSizedVec.data(), preSizedVec.size());
  auto state = bitsery::quickDeserialization<bitsery::InputBufferAdapter<std::vector<uint8_t>>>
    ({ preSizedVec.begin(), preSizedVec.size() }, t);

  if (!state.second) {
    printf("Deserialise fail!\n");
    return false;
  }

  return true;
}

}

std::future<DeserialisedSceneData> SceneSerializer::deserialize(const std::filesystem::path& path)
{
  std::promise<DeserialisedSceneData> promise;
  auto fut = promise.get_future();

  if (_deserialisationThread.joinable()) {
    _deserialisationThread.join();
  }

  _deserialisationThread = std::thread(
    [p = std::move(promise), path, version = g_CurrVersion] () mutable
    {
      std::ifstream file(path, std::ios::binary | std::ios::ate);

      if (file.bad()) {
        printf("Could not open scene for deserialisation (%s)!\n", path.string().c_str());

        p.set_value(DeserialisedSceneData());
        file.close();
        return;
      }

      DeserialisedSceneData outputData{};
      outputData._scene = std::make_unique<render::scene::Scene>();

      auto fileSize = static_cast<uint32_t>(file.tellg());
      file.seekg(0);
      auto headerSz = headerSize();
      std::vector<uint8_t> header(headerSz);
      file.read((char*)header.data(), headerSz);

      assert(header.size() == headerSz && "Deserialised header is not correct size!");

      uint8_t ver = header[0];

      if (ver != version) {
        printf("Cannot deserialise wrong version!\n");
        p.set_value(DeserialisedSceneData());
        return;
      }

      uint32_t* header4BytePtr = reinterpret_cast<uint32_t*>(header.data() + 1);
      uint32_t prefabIdx = header4BytePtr[0];
      uint32_t textureIdx = header4BytePtr[1];
      uint32_t modelIdx = header4BytePtr[2];
      uint32_t materialIdx = header4BytePtr[3];
      uint32_t animationIdx = header4BytePtr[4];
      uint32_t tiIdx = header4BytePtr[5];
      uint32_t nodesIdx = header4BytePtr[6];
      uint32_t cineIdx = header4BytePtr[7];

      std::vector<std::uint8_t> serialisedPrefabs(textureIdx - prefabIdx);
      std::vector<std::uint8_t> serialisedTextures(modelIdx - textureIdx);
      std::vector<std::uint8_t> serialisedModels(materialIdx - modelIdx);
      std::vector<std::uint8_t> serialisedMats(animationIdx - materialIdx);
      std::vector<std::uint8_t> serialisedAnimations(tiIdx - animationIdx);
      std::vector<std::uint8_t> serialisedTis(nodesIdx - tiIdx);
      std::vector<std::uint8_t> serialisedNodes(cineIdx - nodesIdx);
      std::vector<std::uint8_t> serialisedCinematics(fileSize - cineIdx);

      // We need these here so that we can add them properly to the scene.
      std::vector<render::asset::Prefab> prefabs;
      std::vector<render::asset::Texture> textures;
      std::vector<render::asset::Model> models;
      std::vector<render::asset::Material> mats;
      std::vector<render::anim::Animation> animations;
      std::vector<render::scene::Scene::TileInfo> tis;
      std::vector<IntermediateNode> imNodes;
      std::vector<render::asset::Cinematic> cinematics;

      // Read prefabs
      if (!desHelper(file, prefabIdx, serialisedPrefabs, prefabs)) {
        p.set_value(DeserialisedSceneData());
        return;
      }

      // Read textures
      if (!desHelper(file, textureIdx, serialisedTextures, textures)) {
        p.set_value(DeserialisedSceneData());
        return;
      }

      // Read models
      if (!desHelper(file, modelIdx, serialisedModels, models)) {
        p.set_value(DeserialisedSceneData());
        return;
      }

      // Read materials
      if (!desHelper(file, materialIdx, serialisedMats, mats)) {
        p.set_value(DeserialisedSceneData());
        return;
      }

      // Read animations
      if (!desHelper(file, animationIdx, serialisedAnimations, animations)) {
        p.set_value(DeserialisedSceneData());
        return;
      }

      // Read tile infos
      if (!desHelper(file, tiIdx, serialisedTis, tis)) {
        p.set_value(DeserialisedSceneData());
        return;
      }

      // Read intermediate nodes
      if (!desHelper(file, nodesIdx, serialisedNodes, imNodes)) {
        p.set_value(DeserialisedSceneData());
        return;
      }

      // Read cinematics
      if (!desHelper(file, cineIdx, serialisedCinematics, cinematics)) {
        printf("Failed deserialising cinematics\n");
        cinematics.clear();
        //p.set_value(DeserialisedSceneData());
        //return;
      }

      for (auto& p : prefabs) {
        outputData._scene->addPrefab(std::move(p));
      }
      for (auto& t : textures) {
        outputData._scene->addTexture(std::move(t));
      }
      for (auto& model : models) {
        outputData._scene->addModel(std::move(model));
      }
      for (auto& mat : mats) {
        outputData._scene->addMaterial(std::move(mat));
      }
      for (auto& anim : animations) {
        outputData._scene->addAnimation(std::move(anim));
      }
      for (auto& ti : tis) {
        if (ti._ddgiAtlas) {
          outputData._scene->setDDGIAtlas(ti._ddgiAtlas, ti._idx);
        }
      }
      for (auto& cinematic : cinematics) {
        outputData._scene->addCinematic(std::move(cinematic));
      }

      // Translate intermediate node representation to actual nodes
      for (auto& imn : imNodes) {
        auto id = outputData._scene->addNode(std::move(imn._node));

        auto& c = outputData._scene->registry().addComponent<component::Transform>(id);
        c = imn._comps._trans;
        outputData._scene->registry().patchComponent<component::Transform>(id);

        if (imn._comps._rend) {
          auto& c = outputData._scene->registry().addComponent<component::Renderable>(id);
          c = imn._comps._rend.value();
          outputData._scene->registry().patchComponent<component::Renderable>(id);
        }

        if (imn._comps._light) {
          auto& c = outputData._scene->registry().addComponent<component::Light>(id);
          c = imn._comps._light.value();
          outputData._scene->registry().patchComponent<component::Light>(id);
        }

        if (imn._comps._animator) {
          auto& c = outputData._scene->registry().addComponent<component::Animator>(id);
          c = imn._comps._animator.value();
          outputData._scene->registry().patchComponent<component::Animator>(id);
        }

        if (imn._comps._skeleton) {
          auto& c = outputData._scene->registry().addComponent<component::Skeleton>(id);
          c = imn._comps._skeleton.value();
          outputData._scene->registry().patchComponent<component::Skeleton>(id);
        }
      }

      p.set_value(std::move(outputData));
    }
  );

  return fut;
}

}


