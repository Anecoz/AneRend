#pragma once

#include "IGUI.h"
#include "../render/Identifiers.h"

namespace gui {

class EditRenderableGUI : public IGUI
{
public:
  EditRenderableGUI();
  ~EditRenderableGUI();

  void immediateDraw(logic::AneditContext* c) override final;
private:
  int _currentGuizmoOp;
};

}