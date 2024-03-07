#include "AssetCollection.h"

#include "../serialisation/Serialisation.h"

#include <fstream>
#include <future>

namespace {

static std::uint16_t g_LocalDeserialisedVersion = 0;

struct SerialisedAssetInfo
{
  std::uint32_t _offset; // Index into main blob
  std::uint32_t _size;   // Occupied size needed for deserialisation
  util::Uuid _id;
  std::string _name;
  render::asset::AssetMetaInfo::Type _type;
};

}

namespace bitsery {

template <typename S>
void serialize(S& s, SerialisedAssetInfo& p)
{
  s.value4b(p._offset);
  s.value4b(p._size);
  s.object(p._id);
  s.text1b(p._name, 255);
  s.value1b(p._type);
}

template <typename S>
void serialize(S& s, std::vector<SerialisedAssetInfo>& p)
{
  s.container(p, 1000);
}

}

namespace render::asset {

AssetCollection::AssetCollection()
{}

AssetCollection::AssetCollection(std::filesystem::path p)
  : _p(std::move(p))
{}

AssetCollection::~AssetCollection()
{}

AssetCollection::AssetCollection(AssetCollection&& rhs)
{
  if (this != &rhs) {
    _fileIndex = std::move(rhs._fileIndex);
    _metaInfos = std::move(rhs._metaInfos);
    _p = std::move(rhs._p);
    _cachePtr = std::move(rhs._cachePtr);
    _cachedModels = std::move(rhs._cachedModels);
    _cachedMaterials = std::move(rhs._cachedMaterials);
    _cachedPrefabs = std::move(rhs._cachedPrefabs);
    _cachedTextures = std::move(rhs._cachedTextures);
    _cachedCinematics = std::move(rhs._cachedCinematics);
    _cachedAnimations = std::move(rhs._cachedAnimations);
  }
}

AssetCollection& AssetCollection::operator=(AssetCollection&& rhs)
{
  if (this != &rhs) {
    _fileIndex = std::move(rhs._fileIndex);
    _metaInfos = std::move(rhs._metaInfos);
    _p = std::move(rhs._p);
    _cachePtr = std::move(rhs._cachePtr);
    _cachedModels = std::move(rhs._cachedModels);
    _cachedMaterials = std::move(rhs._cachedMaterials);
    _cachedPrefabs = std::move(rhs._cachedPrefabs);
    _cachedTextures = std::move(rhs._cachedTextures);
    _cachedCinematics = std::move(rhs._cachedCinematics);
    _cachedAnimations = std::move(rhs._cachedAnimations);
  }
  return *this;
}

const AssetEventLog& AssetCollection::getEventLog() const
{
  return _log;
}

void AssetCollection::clearEventLog()
{
  _log._events.clear();
}

AssetCollection::operator bool() const
{
  return !_p.empty();
}

const std::vector<AssetMetaInfo>& AssetCollection::getMetaInfos(const AssetMetaInfo::Type type)
{
  return _metaInfos[type];
}

template <typename T>
bool AssetCollection::readIndexAsync(const util::Uuid& id, std::function<void(T)> cb, std::vector<T>& cache)
{
  if (!_fileIndex._map.contains(id)) {
    printf("AssetCollection cannot get asset %s, it doesn't exist in cache or on disk!\n", id.str().c_str());
    return false;
  }

  auto metaInfo = _fileIndex._map[id];
  std::async(std::launch::async, [this, id, &cache, cb, meta = std::move(metaInfo)]() {

    std::ifstream ifs(_p, std::ios::binary);

    std::vector<std::uint8_t> data;
    data.resize(meta._sizeOnDisk);

    ifs.seekg(meta._offset + _indicesFileSize);
    ifs.read((char*)data.data(), meta._sizeOnDisk);
    ifs.close();

    auto m = serialisation::deserializeVector<T>(data);

    if (m) {
      {
        std::lock_guard<std::mutex> lock(_cacheMtx);
        _cachePtr[id] = cache.size();
        cache.emplace_back(m.value());
      }

      cb(std::move(m.value()));
    }
  });

  return true;
}

template<typename T>
T AssetCollection::readIndexBlocking(const util::Uuid& id, std::vector<T>& cache)
{
  if (!_fileIndex._map.contains(id)) {
    printf("AssetCollection cannot get asset %s, it doesn't exist in cache or on disk!\n", id.str().c_str());
    return T{};
  }

  auto metaInfo = _fileIndex._map[id];
  std::ifstream ifs(_p, std::ios::binary);

  std::vector<std::uint8_t> data;
  data.resize(metaInfo._sizeOnDisk);

  ifs.seekg(metaInfo._offset + _indicesFileSize);
  ifs.read((char*)data.data(), metaInfo._sizeOnDisk);
  ifs.close();

  auto m = serialisation::deserializeVector<T>(data);

  if (m) {
    {
      std::lock_guard<std::mutex> lock(_cacheMtx);
      _cachePtr[id] = cache.size();
      cache.emplace_back(m.value());
    }

    return m.value();
  }

  printf("Failed to deserialize asset %s!\n", id.str().c_str());
  return T{};
}

template<typename T>
void AssetCollection::getAssetAsync(const util::Uuid& id, std::function<void(T)> cb, std::vector<T>& cache)
{
  if (_cachePtr.contains(id)) {
    // In cache, return it
    T asset;
    {
      std::lock_guard<std::mutex> lock(_cacheMtx);
      asset = cache[_cachePtr[id]];
    }
    cb(std::move(asset));
  }
  else {
    // Not in cache, request a file read.
    readIndexAsync<T>(id, cb, cache);
  }
}

template<typename T>
T AssetCollection::getAssetBlocking(const util::Uuid& id, std::vector<T>& cache)
{
  if (_cachePtr.contains(id)) {
    // In cache, return it
    T asset;
    {
      std::lock_guard<std::mutex> lock(_cacheMtx);
      asset = cache[_cachePtr[id]];
    }
    return asset;
  }

  return readIndexBlocking<T>(id, cache);
}

template<typename T>
void AssetCollection::addAsset(T asset, std::vector<T>& cache, AssetMetaInfo::Type type)
{
  if (_cachePtr.contains(asset._id)) {
    printf("AssetCollection cannot add asset with id %s, already exists!\n", asset._id.str().c_str());
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  // Add meta info
  AssetMetaInfo meta{};
  meta._type = type;
  meta._id = asset._id;
  meta._name = asset._name;
  _metaInfos[type].emplace_back(std::move(meta));

  _cachePtr[asset._id] = cache.size();
  cache.emplace_back(std::move(asset));
}

template <typename T>
void AssetCollection::removeAsset(const util::Uuid& id, std::vector<T>& cache)
{
  if (!_cachePtr.contains(id)) {
    printf("AssetCollection cannot remove asset, it's not in cache!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  cache.erase(cache.begin() + _cachePtr[id]);

  // Rebuild cache ptr
  _cachePtr.clear();
  for (std::size_t i = 0; i < cache.size(); ++i) {
    _cachePtr[cache[i]._id] = i;
  }
}

template <typename T>
void AssetCollection::updateAsset(T asset, std::vector<T>& cache, AssetMetaInfo::Type type)
{
  if (!_cachePtr.contains(asset._id)) {
    printf("AssetCollection cannot update asset, it doesn't exist in cache!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  // Update in metainfo struct
  auto& vec = _metaInfos[type];
  for (auto& a : vec) {
    if (a._id == asset._id) {
      a._name = asset._name;
      break;
    }
  }

  cache[_cachePtr[asset._id]] = std::move(asset);
}

template<typename T>
std::vector<util::Uuid> AssetCollection::buildCacheId(const std::vector<T>& cache) const
{
  std::vector<util::Uuid> out;

  {
    std::lock_guard<std::mutex> lock(_cacheMtx);
    out.reserve(cache.size());

    for (auto& i : cache) {
      out.emplace_back(i._id);
    }
  }

  return out;
}

void AssetCollection::getModel(const util::Uuid& id, ModelRetrievedCallback cb)
{
  getAssetAsync<Model>(id, cb, _cachedModels);
}

void AssetCollection::add(Model a)
{
  addAsset<Model>(std::move(a), _cachedModels, AssetMetaInfo::Model);
}

void AssetCollection::getMaterial(const util::Uuid& id, MaterialRetrievedCallback cb)
{
  getAssetAsync<Material>(id, cb, _cachedMaterials);
}

void AssetCollection::add(Material a)
{
  addAsset<Material>(std::move(a), _cachedMaterials, AssetMetaInfo::Material);
}

void AssetCollection::getPrefab(const util::Uuid& id, PrefabRetrievedCallback cb)
{
  getAssetAsync<Prefab>(id, cb, _cachedPrefabs);
}

void AssetCollection::add(Prefab a)
{
  addAsset<Prefab>(std::move(a), _cachedPrefabs, AssetMetaInfo::Prefab);
}

void AssetCollection::getTexture(const util::Uuid& id, TextureRetrievedCallback cb)
{
  getAssetAsync<Texture>(id, cb, _cachedTextures);
}

void AssetCollection::add(Texture a)
{
  addAsset<Texture>(std::move(a), _cachedTextures, AssetMetaInfo::Texture);
}

void AssetCollection::getCinematic(const util::Uuid& id, CinematicRetrievedCallback cb)
{
  getAssetAsync<Cinematic>(id, cb, _cachedCinematics);
}

void AssetCollection::getAnimation(const util::Uuid& id, AnimationRetrievedCallback cb)
{
  getAssetAsync<anim::Animation>(id, cb, _cachedAnimations);
}

Model AssetCollection::getModelBlocking(const util::Uuid& id)
{
  return getAssetBlocking(id, _cachedModels);
}

Material AssetCollection::getMaterialBlocking(const util::Uuid& id)
{
  return getAssetBlocking(id, _cachedMaterials);
}

Prefab AssetCollection::getPrefabBlocking(const util::Uuid& id)
{
  return getAssetBlocking(id, _cachedPrefabs);
}

Texture AssetCollection::getTextureBlocking(const util::Uuid& id)
{
  return getAssetBlocking(id, _cachedTextures);
}

Cinematic AssetCollection::getCinematicBlocking(const util::Uuid& id)
{
  return getAssetBlocking(id, _cachedCinematics);
}

anim::Animation AssetCollection::getAnimationBlocking(const util::Uuid& id)
{
  return getAssetBlocking(id, _cachedAnimations);
}

void AssetCollection::add(Cinematic a)
{
  addAsset<Cinematic>(std::move(a), _cachedCinematics, AssetMetaInfo::Cinematic);
}

void AssetCollection::add(anim::Animation a)
{
  addAsset<anim::Animation>(std::move(a), _cachedAnimations, AssetMetaInfo::Animation);
}

void AssetCollection::removeModel(const util::Uuid& id)
{
  removeAsset<Model>(id, _cachedModels);
}

void AssetCollection::updateMaterial(Material a)
{
  addEvent(AssetEventType::MaterialUpdated, a._id);
  updateAsset<Material>(std::move(a), _cachedMaterials, AssetMetaInfo::Type::Material);
}

void AssetCollection::removeMaterial(const util::Uuid& id)
{
  removeAsset<Material>(id, _cachedMaterials);
}

void AssetCollection::updatePrefab(Prefab a)
{
  addEvent(AssetEventType::PrefabUpdated, a._id);
  updateAsset<Prefab>(std::move(a), _cachedPrefabs, AssetMetaInfo::Type::Prefab);
}

void AssetCollection::removePrefab(const util::Uuid& id)
{
  removeAsset<Prefab>(id, _cachedPrefabs);
}

void AssetCollection::updateTexture(Texture a)
{
  addEvent(AssetEventType::TextureUpdated, a._id);
  updateAsset<Texture>(std::move(a), _cachedTextures, AssetMetaInfo::Type::Texture);
}

void AssetCollection::removeTexture(const util::Uuid& id)
{
  removeAsset<Texture>(id, _cachedTextures);
}

void AssetCollection::updateCinematic(Cinematic a)
{
  addEvent(AssetEventType::CinematicUpdated, a._id);
  updateAsset<Cinematic>(std::move(a), _cachedCinematics, AssetMetaInfo::Type::Cinematic);
}

void AssetCollection::removeCinematic(const util::Uuid& id)
{
  removeAsset<Cinematic>(id, _cachedCinematics);
}

void AssetCollection::removeAnimation(const util::Uuid& id)
{
  removeAsset<anim::Animation>(id, _cachedAnimations);
}

namespace {

struct InternalSerialiser
{
  template <typename T>
  void add(const std::vector<T>& vec, render::asset::AssetMetaInfo::Type type)
  {
    for (auto& t : vec) {
      auto data = serialisation::serializeToVector(t);

      _indices.emplace_back(SerialisedAssetInfo{ _currOffset, (std::uint32_t)data.size(), t._id, t._name, type });
      _assetData.insert(_assetData.end(), data.begin(), data.end());

      _currOffset += (std::uint32_t)data.size();
    }
  }

  // Offset includes version and indices size
  std::uint32_t _currOffset = sizeof(g_LocalDeserialisedVersion) + sizeof(std::uint32_t);
  std::vector<SerialisedAssetInfo> _indices;
  std::vector<std::uint8_t> _assetData;
};

}

void AssetCollection::serialiseToPath(std::filesystem::path p)
{
  // Serialise everything that is in cache and disk currently to _p.

  /*
    File structure:
    2 bytes version
    4 bytes indices size
    Indices
    Asset data
    EOF
  */

  if (!p.empty()) {
    _p = std::move(p);
  }

  // Make sure everything on disk (that isn't cached) is also written.
  // To this stupidly by simply reading everything from disk into cache. YOLO.
  for (auto& [type, metaVec] : _metaInfos) {
    for (auto& meta : metaVec) {
      switch (type) {
      case AssetMetaInfo::Model:
        getModelBlocking(meta._id);
        break;
      case AssetMetaInfo::Animation:
        getAnimationBlocking(meta._id);
        break;
      case AssetMetaInfo::Material:
        getMaterialBlocking(meta._id);
        break;
      case AssetMetaInfo::Texture:
        getTextureBlocking(meta._id);
        break;
      case AssetMetaInfo::Prefab:
        getPrefabBlocking(meta._id);
        break;
      case AssetMetaInfo::Cinematic:
        getCinematicBlocking(meta._id);
        break;
      }
    }
  }

  // Open file for writing
  std::ofstream ofs(_p.string(), std::ios::binary);

  if (!ofs.is_open()) {
    printf("AssetCollection could not write to %s, could not open!\n", _p.string().c_str());
    return;
  }

  // Set versions
  g_LocalDeserialisedVersion = g_CurrVersion;
  serialisation::setDeserialisedVersion(g_LocalDeserialisedVersion);

  // Serialise the assets to data vecs
  InternalSerialiser ser;
  ser.add(_cachedModels, AssetMetaInfo::Model);
  ser.add(_cachedMaterials, AssetMetaInfo::Material);
  ser.add(_cachedPrefabs, AssetMetaInfo::Prefab);
  ser.add(_cachedTextures, AssetMetaInfo::Texture);
  ser.add(_cachedCinematics, AssetMetaInfo::Cinematic);
  ser.add(_cachedAnimations, AssetMetaInfo::Animation);

  auto serialisedIndices = serialisation::serializeToVector(ser._indices);
  std::uint32_t indSize = (std::uint32_t)serialisedIndices.size();

  ofs.write((const char*)(&g_LocalDeserialisedVersion), sizeof(g_LocalDeserialisedVersion));
  ofs.write((const char*)(&indSize), sizeof(std::uint32_t));
  ofs.write((const char*)serialisedIndices.data(), serialisedIndices.size());
  ofs.write((const char*)ser._assetData.data(), ser._assetData.size());

  ofs.close();

  printf("AssetCollection serialised to %s\n", _p.string().c_str());
}

void AssetCollection::readIndices()
{
  // Deserialise according to the file structure above, but only read indices
  std::ifstream ifs(_p.string(), std::ios::binary);

  if (!ifs.is_open()) {
    printf("AssetCollection could not open file %s for reading!\n", _p.string().c_str());
    return;
  }

  ifs.seekg(0);

  std::uint16_t ver = 0;
  ifs.read((char*)&ver, sizeof(std::uint16_t));

  g_LocalDeserialisedVersion = ver;
  serialisation::setDeserialisedVersion(g_LocalDeserialisedVersion);

  ifs.read((char*)&_indicesFileSize, sizeof(std::uint32_t));

  // Read indices
  std::vector<std::uint8_t> serialisedIndices;
  serialisedIndices.resize(_indicesFileSize);
  ifs.read((char*)serialisedIndices.data(), _indicesFileSize);

  // Deserialise
  std::optional<std::vector<SerialisedAssetInfo>> opt = serialisation::deserializeVector<std::vector<SerialisedAssetInfo>>(serialisedIndices);

  if (!opt) {
    printf("AssetCollection could not deserialise indices!\n");
    return;
  }

  // Translate indices to structure used in memory
  _fileIndex._map.clear();

  for (auto& info : opt.value()) {
    AssetMetaInfo meta{ info._type, info._id, info._name, info._offset, info._size };

    _metaInfos[meta._type].emplace_back(meta);
    _fileIndex._map[info._id] = std::move(meta);
  }
}

void AssetCollection::printDebugInfo()
{
  // TODO, maybe return string instead? so can print wherever like GUI
}

void AssetCollection::addEvent(AssetEventType type, const util::Uuid& id)
{
  AssetEvent event{};
  event._id = id;
  event._type = type;
  _log._events.emplace_back(std::move(event));
}

}