#include "EditSelectionGUI.h"

#include "EditNodeGUI.h"
#include "EditMaterialGUI.h"
#include "EditPrefabGUI.h"
#include "EditCinematicGUI.h"
#include "EditTextureGUI.h"

#include <imgui.h>

namespace gui {

EditSelectionGUI::EditSelectionGUI()
  : IGUI()
{
  // Fill gui map, will choose correct GUI to render depending on selection type
  _specificGuis[logic::AneditContext::SelectionType::Node] = new EditNodeGUI();
  _specificGuis[logic::AneditContext::SelectionType::Material] = new EditMaterialGUI();
  _specificGuis[logic::AneditContext::SelectionType::Prefab] = new EditPrefabGUI();
  _specificGuis[logic::AneditContext::SelectionType::Texture] = new EditTextureGUI();

  _cinematicGUI = new EditCinematicGUI();
}

EditSelectionGUI::~EditSelectionGUI()
{
  for (auto& item : _specificGuis) {
    delete item.second;
  }

  delete _cinematicGUI;
}

void EditSelectionGUI::immediateDraw(logic::AneditContext* c)
{
  ImGui::Begin("Edit selection");
  if (c->selectionType() != logic::AneditContext::SelectionType::Cinematic) {
    _specificGuis[c->selectionType()]->immediateDraw(c);
  }
  ImGui::End();

  // Always draw cinematic gui
  _cinematicGUI->immediateDraw(c);
}

}