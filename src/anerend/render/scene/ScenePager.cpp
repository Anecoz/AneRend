#include "ScenePager.h"

#include "Scene.h"
#include "Tile.h"
#include "../RenderContext.h"

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

void ScenePager::update(const glm::vec3& pos)
{
  if (!_rc || !_scene) return;

  AssetUpdate upd{};

  // Check event log of Scene to see if any materials, models, anims etc have been added
  const auto& log = _scene->getEvents();
  for (auto& event : log._events) {
    if (event._type == SceneEventType::ModelAdded) {
      auto copy = *_scene->getModel(event._id);
      upd._addedModels.emplace_back(std::move(copy));
    }
    else if (event._type == SceneEventType::TextureAdded) {
      auto copy = *_scene->getTexture(event._id);
      upd._addedTextures.emplace_back(std::move(copy));
    }
    else if (event._type == SceneEventType::MaterialAdded) {
      auto copy = *_scene->getMaterial(event._id);
      upd._addedMaterials.emplace_back(std::move(copy));
    }
    else if (event._type == SceneEventType::MaterialUpdated) {
      auto copy = *_scene->getMaterial(event._id);
      upd._updatedMaterials.emplace_back(std::move(copy));
    }
    else if (event._type == SceneEventType::AnimationAdded) {
      auto copy = *_scene->getAnimation(event._id);
      upd._addedAnimations.emplace_back(std::move(copy));
    }

    // TODO: Removal of assets
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
        for (const auto& event : log._events) {
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
            auto& pagedComp = _scene->registry().getComponent<component::PageStatus>(node);
            pagedComp._paged = false;
            _scene->registry().patchComponent<component::PageStatus>(node);
          }
        }
      }

      for (auto& nodeId : *nodes) {
        if (!_scene->registry().hasComponent<component::PageStatus>(nodeId)) {
          _scene->registry().addComponent<component::PageStatus>(nodeId, true);
        }
        else {
          auto& pagedComp = _scene->registry().getComponent<component::PageStatus>(nodeId);
          pagedComp._paged = true;
        }
        _scene->registry().patchComponent<component::PageStatus>(nodeId);
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
      auto& pagedComp = _scene->registry().getComponent<component::PageStatus>(nodeId);
      pagedComp._paged = false;
      _scene->registry().patchComponent<component::PageStatus>(nodeId);
    }

    // Remove tile info
    upd._removedTileInfos.emplace_back(idx);
  }

  _pagedTiles = std::move(pagedTiles);

  // Do the asset update via rc
  if (upd) {
    _rc->assetUpdate(std::move(upd));
  }
}

}