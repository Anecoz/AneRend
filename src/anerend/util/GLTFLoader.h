#pragma once

#pragma once

#include "../render/asset/Model.h"
#include "../render/asset/Material.h"
#include "../render/animation/Animation.h"
#include "../render/animation/Skeleton.h"

#include <string>
#include <vector>

class GLTFLoader
{
public:
  GLTFLoader() = default;
  ~GLTFLoader() = default;

  static bool loadFromFile(
    const std::string& path,
    render::asset::Model& modelOut,
    std::vector<render::asset::Material>& materialsOut,
    std::vector<int>& materialIndicesOut, // <mesh index in model, material index>
    render::anim::Skeleton& skeletonOut,
    std::vector<render::anim::Animation>& animationsOut);
};

