#pragma once

#include "IGUI.h"

#include "../render/Identifiers.h"

namespace gui {

class SceneListGUI : public IGUI
{
public:
  SceneListGUI();
  ~SceneListGUI();

  void immediateDraw(logic::AneditContext* c) override final;

private:
  render::RenderableId _selectedRenderable = render::INVALID_ID;
};

}