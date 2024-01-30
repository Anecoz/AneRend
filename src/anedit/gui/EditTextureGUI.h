#pragma once

#include "IGUI.h"

namespace gui {

class EditTextureGUI : public IGUI
{
public:
  EditTextureGUI();
  ~EditTextureGUI();

  void immediateDraw(logic::AneditContext* c) override final;
};

}