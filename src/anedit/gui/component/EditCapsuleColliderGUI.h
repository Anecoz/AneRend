#pragma once

#include "../IGUI.h"

namespace gui {

class EditCapsuleColliderGUI : public IGUI
{
public:
  EditCapsuleColliderGUI();
  ~EditCapsuleColliderGUI();

  void immediateDraw(logic::AneditContext* c) override final;
};

}