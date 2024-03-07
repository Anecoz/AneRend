#include "SceneSerializer.h"

#include "../../serialisation/Serialisation.h"
#include "../Scene.h"

#include <cstdint>
#include <fstream>

namespace {

// The currently deserialised version, set in runtime.
std::uint16_t g_LocalDeserialisedVersion = 0;

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

}

namespace render::scene::internal {

namespace {

class InternalSerializer
{
public:
  // Add a type to be serialized
  template <typename T>
  void add(const T object) {
    auto serializedData = serialisation::serializeToVector(object);
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
    2 + // ver
    4 + // tile info idx
    4;  // nodes idx
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
        2 byte  version         (uint16_t)
        4 bytes tileInfo   idx  (uint32_t)
        4 bytes nodes      idx  (uint32_t)

        -- tile infos --

        -- nodes --

        EOF
      */

      // Copy assets. scene is a reference here in order to avoid copying on calling thread,
      // but there currently is no real guarantee that scene is alive here.
      // TODO: Should probably add a mechanism to make sure that scene stays alive for this duration.

      // Set the "deserialisation version" here aswell, so that the serialize() functions
      // use the correct versioning.
      g_LocalDeserialisedVersion = version;
      serialisation::setDeserialisedVersion(g_LocalDeserialisedVersion);

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

        component::forEachPotCompOpt([id = node._id, &scene]<typename T>(std::optional<T>& compOpt) {
          if (scene.registry().hasComponent<T>(id)) {
            compOpt = scene.registry().getComponent<T>(id);
          }
        }, imn._comps);

        imNodes.emplace_back(std::move(imn));
      }

      InternalSerializer ser;
      //ser.add(scene._prefabs);
      //ser.add(scene._textures);
      //ser.add(scene._models);
      //ser.add(scene._materials);
      //ser.add(scene._animations);
      ser.add(scene._tileInfos);
      ser.add(imNodes);
      //ser.add(scene._cinematics);

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

      uint16_t ver = 0;
      std::memcpy(&ver, header.data(), sizeof(std::uint16_t));

      g_LocalDeserialisedVersion = ver;
      serialisation::setDeserialisedVersion(g_LocalDeserialisedVersion);
      printf("Deserialised version %hu, serialisation version is %hu\n", g_LocalDeserialisedVersion, version);

      uint32_t* header4BytePtr = reinterpret_cast<uint32_t*>(header.data() + 2);
      //uint32_t prefabIdx = header4BytePtr[0];
      //uint32_t textureIdx = header4BytePtr[1];
      //uint32_t modelIdx = header4BytePtr[2];
      //uint32_t materialIdx = header4BytePtr[3];
      //uint32_t animationIdx = header4BytePtr[4];
      uint32_t tiIdx = header4BytePtr[0];
      uint32_t nodesIdx = header4BytePtr[1];
      //uint32_t cineIdx = header4BytePtr[7];

      /*std::vector<std::uint8_t> serialisedPrefabs(textureIdx - prefabIdx);
      std::vector<std::uint8_t> serialisedTextures(modelIdx - textureIdx);
      std::vector<std::uint8_t> serialisedModels(materialIdx - modelIdx);
      std::vector<std::uint8_t> serialisedMats(animationIdx - materialIdx);
      std::vector<std::uint8_t> serialisedAnimations(tiIdx - animationIdx);*/
      std::vector<std::uint8_t> serialisedTis(nodesIdx - tiIdx);
      std::vector<std::uint8_t> serialisedNodes(fileSize - nodesIdx);
      //std::vector<std::uint8_t> serialisedCinematics(fileSize - cineIdx);

      // We need these here so that we can add them properly to the scene.
      std::vector<render::asset::Prefab> prefabs;
      std::vector<render::asset::Texture> textures;
      std::vector<render::asset::Model> models;
      std::vector<render::asset::Material> mats;
      std::vector<render::anim::Animation> animations;
      std::vector<render::scene::Scene::TileInfo> tis;
      std::vector<IntermediateNode> imNodes;
      std::vector<render::asset::Cinematic> cinematics;

#if 0
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
#endif

      // Read tile infos
      if (!desHelper(file, tiIdx, serialisedTis, tis)) {
        p.set_value(DeserialisedSceneData());
        return;
      }

      // Read intermediate nodes
      if (!desHelper(file, nodesIdx, serialisedNodes, imNodes)) {
        printf("Failed deserialising nodes\n");
        imNodes.clear();
        //p.set_value(DeserialisedSceneData());
        //return;
      }

#if 0
      // Read cinematics
      if (!desHelper(file, cineIdx, serialisedCinematics, cinematics)) {
        printf("Failed deserialising cinematics\n");
        cinematics.clear();
        //p.set_value(DeserialisedSceneData());
        //return;
      }
#endif

      /*for (auto& p : prefabs) {
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
      }*/
      for (auto& ti : tis) {
        if (ti._ddgiAtlas) {
          outputData._scene->setDDGIAtlas(ti._ddgiAtlas, ti._idx);
        }
      }
      /*for (auto& cinematic : cinematics) {
        outputData._scene->addCinematic(std::move(cinematic));
      }*/

      // Translate intermediate node representation to actual nodes
      for (auto& imn : imNodes) {
        auto id = outputData._scene->addNode(std::move(imn._node));

        auto& c = outputData._scene->registry().addComponent<component::Transform>(id);
        c = imn._comps._trans;
        outputData._scene->registry().patchComponent<component::Transform>(id);

        component::forEachExistingPotComp([&outputData, &id]<typename T>(const T& comp) {
          auto& c = outputData._scene->registry().addComponent<T>(id);
          c = comp;
          outputData._scene->registry().patchComponent<T>(id);
        }, imn._comps);
      }

      p.set_value(std::move(outputData));
    }
  );

  return fut;
}

}


