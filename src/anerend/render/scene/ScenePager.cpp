#include "ScenePager.h"

#include "Scene.h"
#include "Tile.h"
#include "../RenderContext.h"
#include "../asset/AssetCollection.h"

#include <algorithm>

namespace render::scene {

ScenePager::ScenePager()
  : _rc(nullptr)
  , _scene(nullptr)
{}

ScenePager::ScenePager(render::RenderContext* rc)
  : _rc(rc)
  , _scene(nullptr)
{}

ScenePager::~ScenePager()
{}

void ScenePager::setScene(Scene* scene)
{
  _scene = scene;
}

void ScenePager::setAssetCollection(asset::AssetCollection* assColl)
{
  _assColl = assColl;
}

void ScenePager::update(const glm::vec3& pos)
{
  if (!_rc || !_scene) return;

  AssetUpdate upd{};
  std::vector<util::Uuid> nodesToPage;
  std::vector<util::Uuid> nodesToUnpage;

  const auto& sceneLog = _scene->getEvents();
  const auto& assetLog = _assColl->getEventLog();

  // Check for asset udpates.
  // Note: We just assume that it is in cache at this point and do the blocking call to get the asset.
  for (const auto& event : assetLog._events) {
    if (event._type == asset::AssetEventType::MaterialUpdated) {
      upd._updatedMaterials.emplace_back(_assColl->getMaterialBlocking(event._id));
    }
    else if (event._type == asset::AssetEventType::TextureUpdated) {
      upd._updatedTextures.emplace_back(_assColl->getTextureBlocking(event._id));
    }
  }

  // Page this tile + _pageRadius tiles around it
  std::vector<render::scene::TileIndex> pagedTiles;
  auto idx = Tile::posToIdx(pos);
  Tile* tile = nullptr;

  for (int x = idx._idx.x - _pageRadius; x <= idx._idx.x + _pageRadius; ++x) {
    for (int y = idx._idx.y - _pageRadius; y <= idx._idx.y + _pageRadius; ++y) {

      TileIndex currIdx(x, y);

      if (!_scene->getTile(currIdx, &tile)) {
        continue;
      }

      // Is this tile already paged?
      bool dirty = false;
      if (std::find(_pagedTiles.begin(), _pagedTiles.end(), currIdx) != _pagedTiles.end()) {

        // Go through events to see if anything changed on this already paged tile
        for (const auto& event : sceneLog._events) {
          if (event._tileIdx && event._tileIdx == currIdx) {
            if (event._type == SceneEventType::DDGIAtlasAdded) {
              asset::TileInfo ti{};
              ti._index = currIdx;
              ti._ddgiAtlas = tile->getDDGIAtlas();
              upd._addedTileInfos.emplace_back(std::move(ti));
            }
          }
        }

        // If tile is dirty, redo the nodes. If not dirty, be happy here
        if (!tile->dirty()) {
          pagedTiles.emplace_back(currIdx);
          continue;
        }
        
        dirty = true;
      }

      // Page nodes
      std::vector<util::Uuid>* nodes = nullptr;
      if (dirty) {
        nodes = &tile->getDirtyNodes();
      }
      else {
        nodes = &tile->getNodes();
      }

      // If dirty, also check if there are any removed nodes that should be paged out
      if (dirty) {
        for (auto& node : tile->getRemovedNodes()) {
          if (_scene->registry().hasComponent<component::PageStatus>(node)) {
            nodesToUnpage.emplace_back(node);
          }
        }
      }

      for (auto& nodeId : *nodes) {
        nodesToPage.emplace_back(nodeId);
      }

      // Do DDGI Atlas if present
      // TODO: All these should probably be in the TileInfo thing?
      // event::TileInfoUpdated etc instead of individual like the DDGIAtlas?
      if (auto atlasId = tile->getDDGIAtlas()) {
        asset::TileInfo ti{};
        ti._index = currIdx;
        ti._ddgiAtlas = atlasId;
        upd._addedTileInfos.emplace_back(std::move(ti));
      }

      // Reset dirty and add it to the currently paged tiles
      tile->dirty() = false;
      tile->getDirtyNodes().clear();
      tile->getRemovedNodes().clear();
      pagedTiles.emplace_back(currIdx);
    }
  }

  // Page out if _pagedTiles contains tiles that should no longer be paged
  std::vector<render::scene::TileIndex> diff;
  for (auto& prevIdx : _pagedTiles) {
    if (std::find(pagedTiles.begin(), pagedTiles.end(), prevIdx) == pagedTiles.end()) {
      diff.emplace_back(prevIdx);
    }
  }

  // diff contains previously paged tiles that are no longer paged now
  for (auto& idx : diff) {
    if (!_scene->getTile(idx, &tile)) continue;

    // Remove paged component
    for (const auto& nodeId : tile->getNodes()) {
      // Terrain is always paged
      if (_scene->registry().hasComponent<component::Terrain>(nodeId)) continue;

      nodesToUnpage.emplace_back(nodeId);
    }

    // Remove tile info
    upd._removedTileInfos.emplace_back(idx);
  }

  // Check if there are any added terrains that don't have PageStatus yet
  auto terrainView = _scene->registry().getEnttRegistry().view<component::Terrain>(entt::exclude<component::PageStatus>);
  for (auto ent : terrainView) {
    auto id = _scene->registry().reverseLookup(ent);
    nodesToPage.emplace_back(id);
  }

  _pagedTiles = std::move(pagedTiles);

  // Do the page/unpage for all collected nodes
  for (auto& node : nodesToPage) {
    page(node);
  }

  for (auto& node : nodesToUnpage) {
    unpage(node);
  }

  // Do the asset update via rc
  if (upd) {
    _rc->assetUpdate(std::move(upd));
  }
}

void ScenePager::page(const util::Uuid& node)
{

  if (!_scene->registry().hasComponent<component::PageStatus>(node)) {
    _scene->registry().addComponent<component::PageStatus>(node, true);
  }
  else {
    auto& pagedComp = _scene->registry().getComponent<component::PageStatus>(node);
    pagedComp._paged = true;
  }
  _scene->registry().patchComponent<component::PageStatus>(node);
}

void ScenePager::unpage(const util::Uuid& node)
{
  auto& pagedComp = _scene->registry().getComponent<component::PageStatus>(node);
  pagedComp._paged = false;
  _scene->registry().patchComponent<component::PageStatus>(node);
}

}