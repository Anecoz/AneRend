#pragma once

#include "../IGUI.h"

namespace gui {

class EditMeshColliderGUI : public IGUI
{
public:
  EditMeshColliderGUI();
  ~EditMeshColliderGUI();

  void immediateDraw(logic::AneditContext* c) override final;
};

}
