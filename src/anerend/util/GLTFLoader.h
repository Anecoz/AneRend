#pragma once

#pragma once

#include "../render/asset/Model.h"
#include "../render/asset/Material.h"
#include "../render/asset/Prefab.h"
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
    render::asset::Prefab& prefabOut,
    render::asset::Model& modelOut,
    std::vector<render::asset::Material>& materialsOut,
    std::vector<int>& materialIndicesOut,
    render::anim::Skeleton& skeletonOut,
    std::vector<render::anim::Animation>& animationsOut);
};

}



