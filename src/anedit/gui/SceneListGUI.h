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
};

}