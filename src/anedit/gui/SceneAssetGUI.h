#pragma once

#include "IGUI.h"
#include <util/Uuid.h>

namespace gui {

class SceneAssetGUI : public IGUI
{
public:
  SceneAssetGUI();
  ~SceneAssetGUI();

  void immediateDraw(logic::AneditContext* c) override final;

private:
  void loadGLTFClicked(logic::AneditContext* c);
  void nodeDroppedOnPrefab(logic::AneditContext* c, util::Uuid node);

  bool _draggingPrefab = false;
  util::Uuid _draggedPrefab;

  std::string _matFilter;
  std::string _modelFilter;
  std::string _animationFilter;
  std::string _skeletonFilter;
  std::string _prefabFilter;
};

}