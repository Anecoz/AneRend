#pragma once

#include "IGUI.h"

#include "../logic/AneditContext.h"

#include <unordered_map>

namespace gui {

class EditSelectionGUI : public IGUI
{
public:
  EditSelectionGUI();
  ~EditSelectionGUI();

  void immediateDraw(logic::AneditContext* c) override final;

private:
  std::unordered_map<logic::AneditContext::SelectionType, IGUI*> _specificGuis;
};

}