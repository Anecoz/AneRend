#pragma once

#include "../IGUI.h"

namespace gui {

class EditBoxColliderGUI : public IGUI
{
public:
  EditBoxColliderGUI();
  ~EditBoxColliderGUI();

  void immediateDraw(logic::AneditContext* c) override final;
};

}
