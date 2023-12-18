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
    else if (event._type == SceneEventType::AnimatorAdded) {
      auto copy = *_scene->getAnimator(event._id);
      upd._addedAnimators.emplace_back(std::move(copy));
    }
    else if (event._type == SceneEventType::AnimatorUpdated) {
      auto copy = *_scene->getAnimator(event._id);
      upd._updatedAnimators.emplace_back(std::move(copy));
    }
    else if (event._type == SceneEventType::SkeletonAdded) {
      auto copy = *_scene->getSkeleton(event._id);
      upd._addedSkeletons.emplace_back(std::move(copy));
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
      if (std::find(_pagedTiles.begin(), _pagedTiles.end(), currIdx) != _pagedTiles.end()) {

        // Go through events to see if anything changed on this already paged tile
        for (const auto& event : log._events) {
          if (event._tileIdx && event._tileIdx == currIdx) {
            if (event._type == SceneEventType::RenderableAdded) {
              auto rendCopy = *_scene->getRenderable(event._id);
              upd._addedRenderables.emplace_back(std::move(rendCopy));
            }
            else if (event._type == SceneEventType::RenderableUpdated) {
              auto rendCopy = *_scene->getRenderable(event._id);
              upd._updatedRenderables.emplace_back(std::move(rendCopy));
            }
            else if (event._type == SceneEventType::RenderableRemoved) {
              upd._removedRenderables.emplace_back(event._id);
            }
            else if (event._type == SceneEventType::LightAdded) {
              auto lightCopy = *_scene->getLight(event._id);
              upd._addedLights.emplace_back(std::move(lightCopy));
            }
            else if (event._type == SceneEventType::LightUpdated) {
              auto c = *_scene->getLight(event._id);
              upd._updatedLights.emplace_back(std::move(c));
            }
            else if (event._type == SceneEventType::LightRemoved) {
              upd._removedLights.emplace_back(event._id);
            }
            else if (event._type == SceneEventType::DDGIAtlasAdded) {
              asset::TileInfo ti{};
              ti._index = currIdx;
              ti._ddgiAtlas = tile->getDDGIAtlas();
              upd._addedTileInfos.emplace_back(std::move(ti));
            }
          }
        }

        pagedTiles.emplace_back(currIdx);
        continue;
      }

      // Page renderables, lights etc
      for (const auto& rendId : tile->getRenderableIds()) {
        auto copy = *_scene->getRenderable(rendId);
        upd._addedRenderables.emplace_back(std::move(copy));
      }
      for (const auto& lightId : tile->getLightIds()) {
        auto copy = *_scene->getLight(lightId);
        upd._addedLights.emplace_back(std::move(copy));
      }

      // Do DDGI Atlas if present
      if (auto atlasId = tile->getDDGIAtlas()) {
        asset::TileInfo ti{};
        ti._index = currIdx;
        ti._ddgiAtlas = atlasId;
        upd._addedTileInfos.emplace_back(std::move(ti));
      }

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

    // Remove renderables contained in tile
    for (const auto& rendId : tile->getRenderableIds()) {
      upd._removedRenderables.emplace_back(rendId);
    }
    // Remove lights
    for (const auto& lightId : tile->getLightIds()) {
      upd._removedLights.emplace_back(lightId);
    }

    // Remove tile info
    upd._removedTileInfos.emplace_back(idx);
  }

  _pagedTiles = std::move(pagedTiles);

  // Filter out copies of the same renderable update
  if (upd._updatedRenderables.size() > 1) {
    upd._updatedRenderables.erase(
      std::unique(upd._updatedRenderables.begin(), upd._updatedRenderables.end(),
        [](const render::asset::Renderable& a, const render::asset::Renderable& b) {
          return a._id == b._id;
        }), upd._updatedRenderables.end());
  }

  // Do the asset update via rc
  if (upd) {
    _rc->assetUpdate(std::move(upd));
  }
}

}