#pragma once

#include <filesystem>

#include <util/Uuid.h>
#include <render/Camera.h>

namespace render::scene { class Scene; }

namespace logic {

struct AneditContext
{
  virtual ~AneditContext() {}

  virtual render::scene::Scene& scene() = 0;
  virtual void serializeScene() = 0;
  virtual void loadSceneFrom(std::filesystem::path p) = 0;
  virtual void setScenePath(std::filesystem::path p) = 0;

  virtual void startLoadGLTF(std::filesystem::path p) = 0;

  virtual void spawnFromPrefabAtMouse(const util::Uuid& prefab) = 0;

  virtual util::Uuid& selectedRenderable() = 0;

  virtual render::Camera& camera() = 0;

protected:
  util::Uuid _selectedRenderable;
};

}