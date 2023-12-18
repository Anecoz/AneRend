#pragma once

#include "../../util/Uuid.h"
#include "../scene/TileIndex.h"

namespace render::asset {

struct TileInfo
{
  scene::TileIndex _index;

  util::Uuid _ddgiAtlas;
};

}