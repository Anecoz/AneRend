#pragma once

#include "../IGUI.h"

namespace gui {

  class EditCharacterControllerGUI : public IGUI
  {
  public:
    EditCharacterControllerGUI();
    ~EditCharacterControllerGUI();

    void immediateDraw(logic::AneditContext* c) override final;
  };

}
