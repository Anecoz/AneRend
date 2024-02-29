#include "AssetCollection.h"

#include "../serialisation/Serialisation.h"

#include <fstream>
#include <future>

namespace {

static std::uint16_t g_LocalDeserialisedVersion = 0;

struct AssetIndexPair
{
  std::uint32_t _offset; // Index into main blob
  std::uint32_t _size;   // Occupied size needed for deserialisation
  util::Uuid _id;
};

}

namespace bitsery {

template <typename S>
void serialize(S& s, AssetIndexPair& p)
{
  s.value4b(p._offset);
  s.value4b(p._size);
  s.object(p._id);
}

template <typename S>
void serialize(S& s, std::vector<AssetIndexPair>& p)
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
    _p = std::move(rhs._p);
    _cachePtr = std::move(rhs._cachePtr);
    _cachedModels = std::move(rhs._cachedModels);
    _cachedMaterials = std::move(rhs._cachedMaterials);
    _cachedPrefabs = std::move(rhs._cachedPrefabs);
    _cachedTextures = std::move(rhs._cachedTextures);
    _cachedCinematics = std::move(rhs._cachedCinematics);
  }
}

AssetCollection& AssetCollection::operator=(AssetCollection&& rhs)
{
  if (this != &rhs) {
    _fileIndex = std::move(rhs._fileIndex);
    _p = std::move(rhs._p);
    _cachePtr = std::move(rhs._cachePtr);
    _cachedModels = std::move(rhs._cachedModels);
    _cachedMaterials = std::move(rhs._cachedMaterials);
    _cachedPrefabs = std::move(rhs._cachedPrefabs);
    _cachedTextures = std::move(rhs._cachedTextures);
    _cachedCinematics = std::move(rhs._cachedCinematics);
  }
  return *this;
}

AssetCollection::operator bool() const
{
  return !_p.empty();
}

template <typename T>
bool AssetCollection::readIndex(const util::Uuid& id, std::function<void(T)> cb, std::vector<T>& cache)
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

    ifs.seekg(meta._offset);
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
void AssetCollection::getAsset(const util::Uuid& id, std::function<void(T)> cb, std::vector<T>& cache)
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
    readIndex<T>(id, cb, cache);
  }
}

template<typename T>
void AssetCollection::addAsset(T asset, std::vector<T>& cache)
{
  if (_cachePtr.contains(asset._id)) {
    printf("AssetCollection cannot add asset with id %s, already exists!\n", asset._id.str().c_str());
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
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
void AssetCollection::updateAsset(T asset, std::vector<T>& cache)
{
  if (!_cachePtr.contains(asset._id)) {
    printf("AssetCollection cannot update asset, it doesn't exist in cache!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  cache[_cachePtr[asset._id]] = std::move(asset);
}

void AssetCollection::getModel(const util::Uuid& id, ModelRetrievedCallback cb)
{
  getAsset<Model>(id, cb, _cachedModels);
}

void AssetCollection::add(Model a)
{
  addAsset<Model>(std::move(a), _cachedModels);
}

void AssetCollection::getMaterial(const util::Uuid& id, MaterialRetrievedCallback cb)
{
  getAsset<Material>(id, cb, _cachedMaterials);
}

void AssetCollection::add(Material a)
{
  addAsset<Material>(std::move(a), _cachedMaterials);
}

void AssetCollection::getPrefab(const util::Uuid& id, PrefabRetrievedCallback cb)
{
  getAsset<Prefab>(id, cb, _cachedPrefabs);
}

void AssetCollection::add(Prefab a)
{
  addAsset<Prefab>(std::move(a), _cachedPrefabs);
}

void AssetCollection::getTexture(const util::Uuid& id, TextureRetrievedCallback cb)
{
  getAsset<Texture>(id, cb, _cachedTextures);
}

void AssetCollection::add(Texture a)
{
  addAsset<Texture>(std::move(a), _cachedTextures);
}

void AssetCollection::getCinematic(const util::Uuid& id, CinematicRetrievedCallback cb)
{
  getAsset<Cinematic>(id, cb, _cachedCinematics);
}

void AssetCollection::add(Cinematic a)
{
  addAsset<Cinematic>(std::move(a), _cachedCinematics);
}

void AssetCollection::removeModel(const util::Uuid& id)
{
  removeAsset<Model>(id, _cachedModels);
}

void AssetCollection::updateMaterial(Material a)
{
  updateAsset<Material>(std::move(a), _cachedMaterials);
}

void AssetCollection::removeMaterial(const util::Uuid& id)
{
  removeAsset<Material>(id, _cachedMaterials);
}

void AssetCollection::updatePrefab(Prefab a)
{
  updateAsset<Prefab>(std::move(a), _cachedPrefabs);
}

void AssetCollection::removePrefab(const util::Uuid& id)
{
  removeAsset<Prefab>(id, _cachedPrefabs);
}

void AssetCollection::updateTexture(Texture a)
{
  updateAsset<Texture>(std::move(a), _cachedTextures);
}

void AssetCollection::removeTexture(const util::Uuid& id)
{
  removeAsset<Texture>(id, _cachedTextures);
}

void AssetCollection::updateCinematic(Cinematic a)
{
  updateAsset<Cinematic>(std::move(a), _cachedCinematics);
}

void AssetCollection::removeCinematic(const util::Uuid& id)
{
  removeAsset<Cinematic>(id, _cachedCinematics);
}

namespace {

struct InternalSerialiser
{
  template <typename T>
  void add(const std::vector<T>& vec)
  {
    for (auto& t : vec) {
      auto data = serialisation::serializeToVector(t);

      _indices.emplace_back(AssetIndexPair{ _currOffset, (std::uint32_t)data.size(), t._id });
      _assetData.insert(_assetData.end(), data.begin(), data.end());

      _currOffset += (std::uint32_t)data.size();
    }
  }

  std::uint32_t _currOffset = sizeof(g_LocalDeserialisedVersion);
  std::vector<AssetIndexPair> _indices;
  std::vector<std::uint8_t> _assetData;
};

}

void AssetCollection::serialiseCacheToPath()
{
  // Serialise everything that is in cache currently to _p.

  /*
    File structure:
    2 bytes version
    4 bytes indices size
    Indices
    Asset data
    EOF
  */

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
  ser.add(_cachedModels);
  ser.add(_cachedMaterials);
  ser.add(_cachedPrefabs);
  ser.add(_cachedTextures);
  ser.add(_cachedCinematics);

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

  std::uint32_t indSize = 0;
  ifs.read((char*)&indSize, sizeof(std::uint32_t));

  // Read indices
  std::vector<std::uint8_t> serialisedIndices;
  serialisedIndices.resize(indSize);
  ifs.read((char*)serialisedIndices.data(), indSize);

  // Deserialise
  std::optional<std::vector<AssetIndexPair>> opt = serialisation::deserializeVector<std::vector<AssetIndexPair>>(serialisedIndices);

  if (!opt) {
    printf("AssetCollection could not deserialise indices!\n");
    return;
  }

  // Translate indices to structure used in memory
  _fileIndex._map.clear();

  for (auto& pair : opt.value()) {
    AssetMetaInfo meta{ pair._offset, pair._size };
    _fileIndex._map[pair._id] = std::move(meta);
  }
}

void AssetCollection::printDebugInfo()
{
}

}