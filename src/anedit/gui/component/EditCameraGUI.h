#pragma once

#include "../IGUI.h"

namespace gui {

class EditCameraGUI : public IGUI
{
public:
  EditCameraGUI();
  ~EditCameraGUI();

  void immediateDraw(logic::AneditContext* c) override final;
};

}
