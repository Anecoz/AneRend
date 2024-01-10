#pragma once

#include "IGUI.h"

#include <typeindex>
#include <unordered_map>

namespace gui {

class EditNodeGUI : public IGUI
{
public:
  EditNodeGUI();
  ~EditNodeGUI();

  void immediateDraw(logic::AneditContext* c) override final;

private:
  std::unordered_map<std::type_index, IGUI*> _componentGUIs;
};

}