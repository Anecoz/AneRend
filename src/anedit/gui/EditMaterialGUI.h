#pragma once

#include "IGUI.h"

namespace gui {

class EditMaterialGUI : public IGUI
{
public:
  EditMaterialGUI();
  ~EditMaterialGUI();

  void immediateDraw(logic::AneditContext* c) override final;
};

}