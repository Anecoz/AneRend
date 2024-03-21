#pragma once

#include "../../component/Components.h"
#include "../asset/Cinematic.h"
#include "../asset/Prefab.h"
#include "../asset/Model.h"
#include "../asset/Material.h"
#include "../asset/Mesh.h"
#include "../asset/Texture.h"
#include "../asset/TileInfo.h"
#include "../animation/Animation.h"
#include "../Vertex.h"

#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>
#include <bitsery/traits/array.h>
#include <bitsery/traits/string.h>
#include <bitsery/ext/std_optional.h>

namespace {

// The current version if serialising
constexpr std::uint16_t g_CurrVersion = 5;

std::uint16_t g_DeserialisedVersion = 0;

}

namespace bitsery {

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
  void serialize(S& s, component::Terrain& p)
  {
    s.object(p._heightMap);
    s.object(p._tileIndex);
    s.container(p._baseMaterials);
    s.object(p._blendMap);
    s.object(p._vegetationMap);
    s.value4b(p._mpp);
    s.value4b(p._heightScale);
    s.value4b(p._uvScale);
  }

  template <typename S>
  void serialize(S& s, component::RigidBody& p)
  {
    s.value1b(p._motionType);
    s.value4b(p._friction);
    s.value4b(p._restitution);
    s.value4b(p._linearDamping);
    s.value4b(p._angularDamping);
    s.value4b(p._gravityFactor);
    s.value4b(p._mass);
  }

  template <typename S>
  void serialize(S& s, component::SphereCollider& p)
  {
    s.value4b(p._radius);
  }

  template <typename S>
  void serialize(S& s, component::MeshCollider& p)
  {
    s.value1b(p._dummy);
  }

  template <typename S>
  void serialize(S& s, component::BoxCollider& p)
  {
    s.object(p._halfExtent);
  }

  template <typename S>
  void serialize(S& s, component::CapsuleCollider& p)
  {
    s.value4b(p._halfHeight);
    s.value4b(p._radius);
  }

  template <typename S>
  void serialize(S& s, component::CharacterController& p)
  {
    s.object(p._desiredLinearVelocity);
    s.value4b(p._speed);
    s.value4b(p._jumpSpeed);
    if (g_DeserialisedVersion >= 2) {
      s.value4b(p._mass);
    }
  }

  template <typename S>
  void serialize(S& s, component::Camera& p)
  {
    s.value4b(p._fov);
  }

  template <typename S>
  void serialize(S& s, component::Behaviour& p)
  {
    s.text1b(p._name, 255);
  }

  template <typename S>
  void serialize(S& s, component::PotentialComponents& p)
  {
    s.object(p._trans);
    s.ext(p._rend, bitsery::ext::StdOptional{});
    s.ext(p._light, bitsery::ext::StdOptional{});
    s.ext(p._animator, bitsery::ext::StdOptional{});
    s.ext(p._skeleton, bitsery::ext::StdOptional{});
    s.ext(p._terrain, bitsery::ext::StdOptional{});
    s.ext(p._rigidBody, bitsery::ext::StdOptional{});
    s.ext(p._sphereColl, bitsery::ext::StdOptional{});
    s.ext(p._meshColl, bitsery::ext::StdOptional{});
    s.ext(p._boxColl, bitsery::ext::StdOptional{});
    s.ext(p._capsuleColl, bitsery::ext::StdOptional{});
    s.ext(p._charCon, bitsery::ext::StdOptional{});
    if (g_DeserialisedVersion >= 4) {
      s.ext(p._cam, bitsery::ext::StdOptional{});
    }
    if (g_DeserialisedVersion >= 5) {
      s.ext(p._behaviour, bitsery::ext::StdOptional{});
    }
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
    s.container4b(m._indices, 2000000);
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
    s.text1b(t._name, 100);
    s.value1b(t._format);
    s.value4b(t._width);
    s.value4b(t._height);
    s.value4b(t._numMips);
    s.value1b(t._clampToEdge);
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
    s.text1b(r._name, 100);
    s.object(r._model);
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

namespace serialisation {

template <typename T>
static std::vector<std::uint8_t> serializeToVector(const T& object)
{
  std::vector<std::uint8_t> serializedData;
  auto byteSize = (uint32_t)bitsery::quickSerialization<bitsery::OutputBufferAdapter<std::vector<std::uint8_t>>>(serializedData, object);
  serializedData.resize(byteSize);
  return serializedData;
}

template <typename T>
static std::optional<T> deserializeVector(const std::vector<std::uint8_t>& vec)
{
  T t;

  auto state = bitsery::quickDeserialization<bitsery::InputBufferAdapter<std::vector<uint8_t>>>
    ({ vec.begin(), vec.size() }, t);

  if (!state.second) {
    printf("Failed deserialisation\n");
    return {};
  }

  return t;
}

static void setDeserialisedVersion(std::uint16_t v)
{
  g_DeserialisedVersion = v;
}

}