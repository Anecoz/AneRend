#pragma once

#include "IGUI.h"

namespace gui {

class EditLightGUI : public IGUI
{
public:
  EditLightGUI();
  ~EditLightGUI();

  void immediateDraw(logic::AneditContext* c) override final;
};

}