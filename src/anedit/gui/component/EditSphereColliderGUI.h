#pragma once

#include "../IGUI.h"

namespace gui {

class EditSphereColliderGUI : public IGUI
{
public:
  EditSphereColliderGUI();
  ~EditSphereColliderGUI();

  void immediateDraw(logic::AneditContext* c) override final;
};

}