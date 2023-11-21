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

  util::Uuid _selectedMaterial;
  util::Uuid _selectedModel;
  util::Uuid _selectedAnimation;
  util::Uuid _selectedAnimator;
  util::Uuid _selectedSkeleton;
  util::Uuid _selectedPrefab;
};

}