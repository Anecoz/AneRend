#pragma once

#pragma once

#include "../render/Model.h"

#include <string>
#include <vector>

class GLTFLoader
{
public:
  GLTFLoader() = default;
  ~GLTFLoader() = default;

  static bool loadFromFile(
    const std::string& path,
    render::Model& modelOut);
};

