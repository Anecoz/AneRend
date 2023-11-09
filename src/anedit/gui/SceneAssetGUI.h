#pragma once

#include "IGUI.h"

namespace gui {

class SceneAssetGUI : IGUI
{
public:
  SceneAssetGUI();
  ~SceneAssetGUI();

  void immediateDraw(logic::AneditContext* c) override final;
};

}