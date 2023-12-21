#pragma once

#pragma once

#include "../render/asset/Model.h"
#include "../render/asset/Material.h"
#include "../render/asset/Prefab.h"
#include "../render/asset/Texture.h"
#include "../render/animation/Animation.h"
#include "../render/animation/Skeleton.h"

#include <string>
#include <vector>

namespace util {

class GLTFLoader
{
public:
  GLTFLoader() = default;
  ~GLTFLoader() = default;

  static bool loadFromFile(
    const std::string& path,
    std::vector<render::asset::Prefab>& prefabsOut,
    std::vector<render::asset::Model>& modeslOut,
    std::vector<render::asset::Texture>& texturesOut,
    std::vector<render::asset::Material>& materialsOut,
    std::vector<render::anim::Skeleton>& skeletonsOut,
    std::vector<render::anim::Animation>& animationsOut);
};

}



