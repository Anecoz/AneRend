#pragma once

#include "IGUI.h"

#include <util/Uuid.h>

namespace gui {

class SceneListGUI : public IGUI
{
public:
  SceneListGUI();
  ~SceneListGUI();

  void immediateDraw(logic::AneditContext* c) override final;

private:
  util::Uuid _selectedRenderable;
  util::Uuid _selectedAnimator;
  util::Uuid _selectedPrefab;
  util::Uuid _selectedLight;

  void addFromPrefab(logic::AneditContext* c);
  void saveSceneAsClicked(logic::AneditContext* c);
  void saveSceneClicked(logic::AneditContext* c);
  void loadSceneClicked(logic::AneditContext* c);
  void addAnimatorClicked(logic::AneditContext* c);
  void addLightClicked(logic::AneditContext* c);
};

}