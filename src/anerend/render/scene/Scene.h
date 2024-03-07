#pragma once

#include "Tile.h"
#include "TileIndex.h"
#include "DeserialisedSceneData.h"
#include "Node.h"

#include "../../component/Components.h"
#include "../../component/Registry.h"

#include "../../util/Uuid.h"
#include "../asset/Model.h"
#include "../asset/Material.h"
#include "../asset/Prefab.h"
#include "../asset/Texture.h"
#include "../asset/Cinematic.h"

#include "../animation/Animation.h"

#include "internal/SceneSerializer.h"

#include <atomic>
#include <filesystem>
#include <future>
#include <memory>
#include <vector>

namespace render::scene {

enum class SceneEventType
{ 
  DDGIAtlasAdded
};

struct SceneEvent
{
  SceneEventType _type;
  TileIndex _tileIdx;
  util::Uuid _id;
};

struct SceneEventLog
{
  std::vector<SceneEvent> _events;
};

class Scene
{
public:
  Scene();
  ~Scene();

  Scene(Scene&&);
  Scene& operator=(Scene&&);

  // Copying is not allowed.
  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;

  void update();

  // This will take a snapshot of the scene as it is at call time,
  // and then asynchronously serialize to path.
  void serializeAsync(const std::filesystem::path& path);

  std::future<DeserialisedSceneData> deserializeAsync(const std::filesystem::path& path);

  const SceneEventLog& getEvents() const;
  void resetEvents();

  bool getTile(TileIndex idx, Tile** tileOut);

  component::Registry& registry() { return _registry; }

  const std::vector<Node>& getNodes() const {
    return _nodeVec;
  }

  util::Uuid addNode(Node node);
  void removeNode(util::Uuid id); // Will recursively remove all (potential) children
  const Node* getNode(util::Uuid id);

  // Node interface
  void setNodeName(util::Uuid& node, std::string name);
  void addNodeChild(util::Uuid& node, util::Uuid& child); // This assumes child didn't have a parent before
  void setNodeAsChild(util::Uuid& node, util::Uuid& child); // This _changes_ parent/children, so previous parent is ok
  void removeNodeChild(util::Uuid& node, util::Uuid& child);
  std::vector<util::Uuid> getNodeChildren(util::Uuid& node);
  component::PotentialComponents nodeToPotComps(util::Uuid& node);
  util::Uuid findRoot(const Node& node);

  void setDDGIAtlas(util::Uuid texId, scene::TileIndex idx);

  struct TileInfo
  {
    TileIndex _idx;
    util::Uuid _ddgiAtlas;
  };

private:
  friend struct internal::SceneSerializer;

  std::unordered_map<TileIndex, Tile> _tiles;
  std::vector<TileInfo> _tileInfos;
  std::unordered_map<util::Uuid, TileIndex> _nodeTileMap; // Helper to know which tile a node has been added to

  struct NodePendingRemoval
  {
    util::Uuid _node;
    int _counter = 0;
  };
  
  std::vector<NodePendingRemoval> _nodesPendingRemoval;

  std::unordered_map<util::Uuid, std::size_t> _nodes; // Index into _nodeVec
  std::vector<scene::Node> _nodeVec;

  component::Registry _registry;

  SceneEventLog _eventLog;
  internal::SceneSerializer _serialiser;

  entt::observer _transformObserver;
  std::atomic_bool _goThroughAllNodes = false;

  std::atomic_uint _atomicPatchIndex = 0;
  std::vector<util::Uuid> _nodesToBePatched;

  void addEvent(SceneEventType type, util::Uuid id, TileIndex tileIdx = TileIndex());
  void updateDependentTransforms(Node& node);
  void updateChildrenTransforms(Node& node);
};

}