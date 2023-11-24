#include "SceneSerializer.h"

#include "../Scene.h"

#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>
#include <bitsery/traits/string.h>

#include <cstdint>
#include <fstream>

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
void serialize(S& s, render::asset::Prefab& p)
{
  s.object(p._id);
  s.text1b(p._name, 40);
  s.object(p._model);
  s.object(p._skeleton);
  s.container(p._materials, 200);
}

template <typename S>
void serialize(S& s, std::vector<render::asset::Prefab>& v)
{
  s.container(v, 100);
}

template <typename S>
void serialize(S& s, render::asset::Mesh& m)
{
  s.object(m._id);
  s.container(m._vertices, 500000);
  s.container4b(m._indices, 500000);
}

template <typename S>
void serialize(S& s, render::asset::Model& m)
{
  s.object(m._id);
  s.text1b(m._name, 40);
  s.container(m._meshes, 200);
}

template <typename S>
void serialize(S& s, std::vector<render::asset::Model>& v)
{
  s.container(v, 100);
}

template <typename S>
void serialize(S& s, render::asset::Material& m)
{
  s.object(m._id);
  s.text1b(m._name, 40);
  s.object(m._baseColFactor);
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
  s.container1b(t._data, 100'000'000); // 100MB
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
  s.container4b(c._inputTimes, 2000);
  s.container(c._outputs, 2000);
}

template <typename S>
void serialize(S& s, render::anim::Animation& a)
{
  s.object(a._id);
  s.text1b(a._name, 40);
  s.container(a._channels, 100);
}

template <typename S>
void serialize(S& s, std::vector<render::anim::Animation>& a)
{
  s.container(a, 100);
}

template <typename S>
void serialize(S& s, render::asset::Animator& a)
{
  s.object(a._id);
  s.text1b(a._name, 40);
  s.value1b(a._state);
  s.object(a._skeleId);
  s.object(a._animId);
  s.value4b(a._playbackMultiplier);
}

template <typename S>
void serialize(S& s, std::vector<render::asset::Animator>& a)
{
  s.container(a, 100);
}

template <typename S>
void serialize(S& s, render::anim::Joint& j)
{
  s.object(j._globalTransform);
  s.object(j._localTransform);
  s.object(j._originalLocalTransform);
  s.object(j._inverseBindMatrix);
  s.value4b(j._internalId);
  s.container4b(j._childrenInternalIds, 50);
}

template <typename S>
void serialize(S& s, render::anim::Skeleton& skel)
{
  s.object(skel._id);
  s.text1b(skel._name, 40);
  s.value1b(skel._nonJointRoot);
  s.container(skel._joints, 100);
}

template <typename S>
void serialize(S& s, std::vector<render::anim::Skeleton>& skel)
{
  s.container(skel, 100);
}

template <typename S>
void serialize(S& s, render::asset::Renderable& r)
{
  s.object(r._id);
  s.text1b(r._name, 40);
  s.object(r._model);
  s.object(r._skeleton);
  s.container(r._materials, 200);
  s.object(r._transform);
  s.object(r._tint);
  s.object(r._boundingSphere);
  s.value1b(r._visible);
}

template <typename S>
void serialize(S& s, std::vector<render::asset::Renderable>& r)
{
  s.container(r, 100'000);
}

}

namespace render::scene::internal {

namespace {

std::uint32_t headerSize()
{
  return
    1 + // ver
    4 + // prefab idx
    4 + // texture idx
    4 + // model idx
    4 + // mat idx
    4 + // anim idx
    4 + // skel idx
    4 + // animator idx
    4; // rend idx
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

void SceneSerializer::serialize(const Scene& scene, const std::filesystem::path& path)
{
  // Copy data from scene and then launch a thread to do the serialisation

  if (_serialisationThread.joinable()) {
    _serialisationThread.join();
  }

  _serialisationThread = std::thread(
    [
      &scene,
      path,
      version = _currentVersion
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
        4 bytes skeleton idx    (uint32_t)
        4 bytes animator idx    (uint32_t)
        4 bytes renderable idx  (uint32_t)

        -- prefabs --

        -- textures --

        -- models --

        -- materials --

        -- animations --

        -- skeletons --

        -- animators --

        -- renderables --

        EOF
      */

      // Copy assets. scene is a reference here in order to avoid copying on calling thread,
      // but there currently is no real guarantee that scene is alive here.
      // TODO: Should probably add a mechanism to make sure that scene stays alive for this duration.
      auto models = scene._models;
      auto prefabs = scene._prefabs;
      auto textures = scene._textures;
      auto materials = scene._materials;
      auto animations = scene._animations;
      auto skeletons = scene._skeletons;
      auto renderables = scene._renderables;
      auto animators = scene._animators;

      std::ofstream file(path, std::ios::binary);
      if (file.bad()) {
        printf("Could not open path to serialize scene: %s!\n", path.string().c_str());
        file.close();
        return;
      }

      printf("Starting scene serialization to %s\n", path.string().c_str());

      // Use bitsery to serialize the vectors
      std::vector<std::uint8_t> serialisedPrefabs;
      std::vector<std::uint8_t> serialisedTextures;
      std::vector<std::uint8_t> serialisedModels;
      std::vector<std::uint8_t> serialisedMats;
      std::vector<std::uint8_t> serialisedAnimations;
      std::vector<std::uint8_t> serialisedSkeletons;
      std::vector<std::uint8_t> serialisedAnimators;
      std::vector<std::uint8_t> serializedRenderables;

      printf("Serialising prefabs...\n");
      auto prefabsByteSize = (uint32_t)bitsery::quickSerialization<bitsery::OutputBufferAdapter<std::vector<std::uint8_t>>>(serialisedPrefabs, prefabs);
      printf("Serialising textures...\n");
      auto texturesByteSize = (uint32_t)bitsery::quickSerialization<bitsery::OutputBufferAdapter<std::vector<std::uint8_t>>>(serialisedTextures, textures);
      printf("Serialising models...\n");
      auto modelsByteSize = (uint32_t)bitsery::quickSerialization<bitsery::OutputBufferAdapter<std::vector<std::uint8_t>>>(serialisedModels, models);
      printf("Serialising materials...\n");
      auto matsByteSize = (uint32_t)bitsery::quickSerialization<bitsery::OutputBufferAdapter<std::vector<std::uint8_t>>>(serialisedMats, materials);
      printf("Serialising animations...\n");
      auto animationsByteSize = (uint32_t)bitsery::quickSerialization<bitsery::OutputBufferAdapter<std::vector<std::uint8_t>>>(serialisedAnimations, animations);
      printf("Serialising skeletons...\n");
      auto skeletonsByteSize = (uint32_t)bitsery::quickSerialization<bitsery::OutputBufferAdapter<std::vector<std::uint8_t>>>(serialisedSkeletons, skeletons);
      printf("Serialising animators...\n");
      auto animatorsByteSize = (uint32_t)bitsery::quickSerialization<bitsery::OutputBufferAdapter<std::vector<std::uint8_t>>>(serialisedAnimators, animators);
      printf("Serialising renderables...\n");
      auto renderablesByteSize = (uint32_t)bitsery::quickSerialization<bitsery::OutputBufferAdapter<std::vector<std::uint8_t>>>(serializedRenderables, renderables);

      auto headerSz = headerSize();

      printf("Writing to disk...\n");

      auto prefabIdx = headerSz;
      auto textureIdx = prefabIdx + prefabsByteSize;
      auto modelIdx = textureIdx + texturesByteSize;
      auto materialIdx = modelIdx + modelsByteSize;
      auto animationIdx = materialIdx + matsByteSize;
      auto skeletonIdx = animationIdx + animationsByteSize;
      auto animatorIdx = skeletonIdx + skeletonsByteSize;
      auto renderableIdx = animatorIdx + animatorsByteSize;

      file.write((const char*)&version, 1);
      // prefab idx (where model info starts)
      file.write((const char*)&prefabIdx, 4);
      // texture idx (where model info starts)
      file.write((const char*)&textureIdx, 4);
      // model idx (where model info starts)
      file.write((const char*)&modelIdx, 4);
      // material idx
      file.write((const char*)&materialIdx, 4);
      // animation idx
      file.write((const char*)&animationIdx, 4);
      // skeleton idx
      file.write((const char*)&skeletonIdx, 4);
      // animator idx
      file.write((const char*)&animatorIdx, 4);
      // renderable idx
      file.write((const char*)&renderableIdx, 4);

      
      // write the data
      file.write((const char*)serialisedPrefabs.data(), prefabsByteSize);
      file.write((const char*)serialisedTextures.data(), texturesByteSize);
      file.write((const char*)serialisedModels.data(), modelsByteSize);
      file.write((const char*)serialisedMats.data(), matsByteSize);
      file.write((const char*)serialisedAnimations.data(), animationsByteSize);
      file.write((const char*)serialisedSkeletons.data(), skeletonsByteSize);
      file.write((const char*)serialisedAnimators.data(), animatorsByteSize);
      file.write((const char*)serializedRenderables.data(), renderablesByteSize);

      file.close();

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
    [p = std::move(promise), path, version = _currentVersion] () mutable
    {
      std::ifstream file(path, std::ios::binary | std::ios::ate);

      if (file.bad()) {
        printf("Could not open scene for deserialisation (%s)!\n", path.string().c_str());

        p.set_value(DeserialisedSceneData());
        file.close();
        return;
      }

      // Read header
      /*
        File structure (version 1):

        -- header--
        1 byte  version         (uint8_t)
        4 bytes prefabs idx     (uint32_t)
        4 bytes tex idx         (uint32_t)
        4 bytes model idx       (uint32_t)
        4 bytes material idx    (uint32_t)
        4 bytes animation idx   (uint32_t)
        4 bytes skeleton idx    (uint32_t)
        4 bytes animator idx    (uint32_t)
        4 bytes renderable idx  (uint32_t)

        -- prefabs --

        -- textures --

        -- models --

        -- materials --

        -- animations --

        -- skeletons --

        -- animators -- 

        -- renderables --

        EOF
      */

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
      uint32_t skeletonIdx = header4BytePtr[5];
      uint32_t animatorIdx = header4BytePtr[6];
      uint32_t rendIdx = header4BytePtr[7];

      std::vector<std::uint8_t> serialisedPrefabs(textureIdx - prefabIdx);
      std::vector<std::uint8_t> serialisedTextures(modelIdx - textureIdx);
      std::vector<std::uint8_t> serialisedModels(materialIdx - modelIdx);
      std::vector<std::uint8_t> serialisedMats(animationIdx - materialIdx);
      std::vector<std::uint8_t> serialisedAnimations(skeletonIdx - animationIdx);
      std::vector<std::uint8_t> serialisedSkeletons(animatorIdx - skeletonIdx);
      std::vector<std::uint8_t> serialisedAnimators(rendIdx - animatorIdx);
      std::vector<std::uint8_t> serialisedRenderables(fileSize - rendIdx);

      // We need these here so that we can add them properly to the scene.
      std::vector<render::asset::Prefab> prefabs;
      std::vector<render::asset::Texture> textures;
      std::vector<render::asset::Model> models;
      std::vector<render::asset::Material> mats;
      std::vector<render::anim::Animation> animations;
      std::vector<render::anim::Skeleton> skeletons;
      std::vector<render::asset::Animator> animators;
      std::vector<render::asset::Renderable> rends;

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

      // Read skeletons
      if (!desHelper(file, skeletonIdx, serialisedSkeletons, skeletons)) {
        p.set_value(DeserialisedSceneData());
        return;
      }

      // Read animators
      if (!desHelper(file, animatorIdx, serialisedAnimators, animators)) {
        p.set_value(DeserialisedSceneData());
        return;
      }

      // Read renderables
      if (!desHelper(file, rendIdx, serialisedRenderables, rends)) {
        p.set_value(DeserialisedSceneData());
        return;
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
      for (auto& skel : skeletons) {
        outputData._scene->addSkeleton(std::move(skel));
      }
      for (auto& animator : animators) {
        outputData._scene->addAnimator(std::move(animator));
      }
      for (auto& r : rends) {
        outputData._scene->addRenderable(std::move(r));
      }

      p.set_value(std::move(outputData));
    }
  );

  return fut;
}

}


