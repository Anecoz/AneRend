#pragma once

#include "../IGUI.h"

namespace gui {

class EditTransformGUI : public IGUI
{
public:
  EditTransformGUI();
  ~EditTransformGUI();

  void immediateDraw(logic::AneditContext* c) override final;
private:
  int _currentGuizmoOp;
  int _currentGuizmoMode;
};

}