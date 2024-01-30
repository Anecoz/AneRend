#pragma once

#include "../IGUI.h"

#include <util/Uuid.h>

namespace gui {

class EditAnimatorGUI : public IGUI
{
public:
  EditAnimatorGUI();
  ~EditAnimatorGUI();

  void immediateDraw(logic::AneditContext* c) override final;
};

}