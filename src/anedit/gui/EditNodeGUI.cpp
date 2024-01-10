#include "EditNodeGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include "EditTransformGUI.h"
#include "EditLightGUI.h"
#include "EditRenderableGUI.h"

namespace gui {

EditNodeGUI::EditNodeGUI()
  : IGUI()
{
  _componentGUIs[typeid(component::Transform)] = new EditTransformGUI();
}

EditNodeGUI::~EditNodeGUI()
{
  for (auto& pair : _componentGUIs) {
    delete pair.second;
  }
}

void EditNodeGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->getFirstSelection();

  if (!id || (c->selectionType() != logic::AneditContext::SelectionType::Node)) {
    return;
  }

  if (c->scene().registry().hasComponent<component::Transform>(id)) {
    _componentGUIs[typeid(component::Transform)]->immediateDraw(c);
  }
}

}