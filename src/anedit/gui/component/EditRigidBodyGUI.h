#pragma once

#include "../IGUI.h"

namespace gui {

class EditRigidBodyGUI : public IGUI
{
public:
  EditRigidBodyGUI();
  ~EditRigidBodyGUI();

  void immediateDraw(logic::AneditContext* c) override final;
};

}