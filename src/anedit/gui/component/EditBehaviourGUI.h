#pragma once

#include "../IGUI.h"

namespace gui {

class EditBehaviourGUI : public IGUI
{
public:
  EditBehaviourGUI();
  ~EditBehaviourGUI();

  void immediateDraw(logic::AneditContext* c) override final;
};

}
