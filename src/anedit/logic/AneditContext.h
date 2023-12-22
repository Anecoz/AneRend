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

  enum class SelectionType
  {
    Renderable,
    Material,
    Prefab,
    Animator,
    Light
  };

  virtual std::vector<util::Uuid>& selection() = 0;
  virtual util::Uuid getFirstSelection() {
    if (_selection.empty()) {
      return util::Uuid();
    }

    return _selection[0];
  }
  virtual SelectionType& selectionType() { return _selectionType; }

  virtual render::Camera& camera() = 0;

protected:
  std::vector<util::Uuid> _selection;
  SelectionType _selectionType = SelectionType::Renderable;
};

}