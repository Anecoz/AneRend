#pragma once

#include "IGUI.h"

namespace gui {

class EditPrefabGUI : public IGUI
{
public:
  EditPrefabGUI();
  ~EditPrefabGUI();

  void immediateDraw(logic::AneditContext* c) override final;
};

}