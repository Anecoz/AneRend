#pragma once

#include "../IGUI.h"

namespace gui {

class EditRenderableGUI : public IGUI
{
public:
  EditRenderableGUI();
  ~EditRenderableGUI();

  void immediateDraw(logic::AneditContext* c) override final;
private:
  const int _bsGuizmoOp;
};

}