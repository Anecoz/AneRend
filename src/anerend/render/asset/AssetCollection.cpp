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

void AssetCollection::add(Model a)
{
  if (_cachePtr.contains(a._id)) {
    printf("AssetCollection will not add model, it already exists!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  _cachePtr[a._id] = _cachedModels.size();
  _cachedModels.emplace_back(std::move(a));
}

void AssetCollection::add(Material a)
{
  if (_cachePtr.contains(a._id)) {
    printf("AssetCollection will not add material, it already exists!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  _cachePtr[a._id] = _cachedMaterials.size();
  _cachedMaterials.emplace_back(std::move(a));
}

void AssetCollection::add(Prefab a)
{
  if (_cachePtr.contains(a._id)) {
    printf("AssetCollection will not add prefab, it already exists!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  _cachePtr[a._id] = _cachedPrefabs.size();
  _cachedPrefabs.emplace_back(std::move(a));
}

void AssetCollection::add(Texture a)
{
  if (_cachePtr.contains(a._id)) {
    printf("AssetCollection will not add texture, it already exists!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  _cachePtr[a._id] = _cachedTextures.size();
  _cachedTextures.emplace_back(std::move(a));
}

void AssetCollection::add(Cinematic a)
{
  if (_cachePtr.contains(a._id)) {
    printf("AssetCollection will not add cinematic, it already exists!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  _cachePtr[a._id] = _cachedCinematics.size();
  _cachedCinematics.emplace_back(std::move(a));
}

void AssetCollection::updateMaterial(Material a)
{
  if (!_cachePtr.contains(a._id)) {
    printf("AssetCollection cannot update material, it doesn't exist in cache!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  _cachedMaterials[_cachePtr[a._id]] = std::move(a);
}

void AssetCollection::updatePrefab(Prefab a)
{
  if (!_cachePtr.contains(a._id)) {
    printf("AssetCollection cannot update prefab, it doesn't exist in cache!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  _cachedPrefabs[_cachePtr[a._id]] = std::move(a);
}

void AssetCollection::updateTexture(Texture a)
{
  if (!_cachePtr.contains(a._id)) {
    printf("AssetCollection cannot update texture, it doesn't exist in cache!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  _cachedTextures[_cachePtr[a._id]] = std::move(a);
}

void AssetCollection::updateCinematic(Cinematic a)
{
  if (!_cachePtr.contains(a._id)) {
    printf("AssetCollection cannot update cinematic, it doesn't exist in cache!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  _cachedCinematics[_cachePtr[a._id]] = std::move(a);
}

void AssetCollection::removeModel(const util::Uuid& id)
{
  if (!_cachePtr.contains(id)) {
    printf("AssetCollection cannot remove model, it's not in cache!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  _cachedModels.erase(_cachedModels.begin() + _cachePtr[id]);

  // Rebuild cache ptr
  _cachePtr.clear();
  for (std::size_t i = 0; i < _cachedModels.size(); ++i) {
    _cachePtr[_cachedModels[i]._id] = i;
  }
}

void AssetCollection::removeMaterial(const util::Uuid& id)
{
  if (!_cachePtr.contains(id)) {
    printf("AssetCollection cannot remove material, it's not in cache!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  _cachedMaterials.erase(_cachedMaterials.begin() + _cachePtr[id]);

  // Rebuild cache ptr
  _cachePtr.clear();
  for (std::size_t i = 0; i < _cachedMaterials.size(); ++i) {
    _cachePtr[_cachedMaterials[i]._id] = i;
  }
}

void AssetCollection::removePrefab(const util::Uuid& id)
{
  if (!_cachePtr.contains(id)) {
    printf("AssetCollection cannot remove prefab, it's not in cache!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  _cachedPrefabs.erase(_cachedPrefabs.begin() + _cachePtr[id]);

  // Rebuild cache ptr
  _cachePtr.clear();
  for (std::size_t i = 0; i < _cachedPrefabs.size(); ++i) {
    _cachePtr[_cachedPrefabs[i]._id] = i;
  }
}

void AssetCollection::removeTexture(const util::Uuid& id)
{
  if (!_cachePtr.contains(id)) {
    printf("AssetCollection cannot remove texture, it's not in cache!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  _cachedTextures.erase(_cachedTextures.begin() + _cachePtr[id]);

  // Rebuild cache ptr
  _cachePtr.clear();
  for (std::size_t i = 0; i < _cachedTextures.size(); ++i) {
    _cachePtr[_cachedTextures[i]._id] = i;
  }
}

void AssetCollection::removeCinematic(const util::Uuid& id)
{
  if (!_cachePtr.contains(id)) {
    printf("AssetCollection cannot remove cinematic, it's not in cache!\n");
    return;
  }

  std::lock_guard<std::mutex> lock(_cacheMtx);
  _cachedCinematics.erase(_cachedCinematics.begin() + _cachePtr[id]);

  // Rebuild cache ptr
  _cachePtr.clear();
  for (std::size_t i = 0; i < _cachedCinematics.size(); ++i) {
    _cachePtr[_cachedCinematics[i]._id] = i;
  }
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

void AssetCollection::getModel(const util::Uuid& id, ModelRetrievedCallback cb)
{
  if (_cachePtr.contains(id)) {
    // In cache, return it
    Model m;
    {
      std::lock_guard<std::mutex> lock(_cacheMtx);
      m = _cachedModels[_cachePtr[id]];
    }
    cb(std::move(m));
  }
  else {
    // Not in cache, request a file read.
    readIndex<Model>(id, cb, _cachedModels);
  }
}

void AssetCollection::getMaterial(const util::Uuid& id, MaterialRetrievedCallback cb)
{
  if (_cachePtr.contains(id)) {
    // In cache, return it
    Material m;
    {
      std::lock_guard<std::mutex> lock(_cacheMtx);
      m = _cachedMaterials[_cachePtr[id]];
    }
    cb(std::move(m));
  }
  else {
    // Not in cache, request a file read.
    readIndex<Material>(id, cb, _cachedMaterials);
  }
}

void AssetCollection::getPrefab(const util::Uuid& id, PrefabRetrievedCallback cb)
{
  if (_cachePtr.contains(id)) {
    // In cache, return it
    Prefab p;
    {
      std::lock_guard<std::mutex> lock(_cacheMtx);
      p = _cachedPrefabs[_cachePtr[id]];
    }
    cb(std::move(p));
  }
  else {
    // Not in cache, request a file read.
    readIndex<Prefab>(id, cb, _cachedPrefabs);
  }
}

void AssetCollection::getTexture(const util::Uuid& id, TextureRetrievedCallback cb)
{
  if (_cachePtr.contains(id)) {
    // In cache, return it
    Texture t;
    {
      std::lock_guard<std::mutex> lock(_cacheMtx);
      t = _cachedTextures[_cachePtr[id]];
    }
    cb(std::move(t));
  }
  else {
    // Not in cache, request a file read.
    readIndex<Texture>(id, cb, _cachedTextures);
  }
}

void AssetCollection::getCinematic(const util::Uuid& id, CinematicRetrievedCallback cb)
{
  if (_cachePtr.contains(id)) {
    // In cache, return it
    Cinematic c;
    {
      std::lock_guard<std::mutex> lock(_cacheMtx);
      c = _cachedCinematics[_cachePtr[id]];
    }
    cb(std::move(c));
  }
  else {
    // Not in cache, request a file read.
    readIndex<Cinematic>(id, cb, _cachedCinematics);
  }
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