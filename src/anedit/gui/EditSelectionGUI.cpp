#include "EditSelectionGUI.h"

//#include "EditAnimatorGUI.h"
#include "EditNodeGUI.h"
#include "EditMaterialGUI.h"
#include "EditPrefabGUI.h"
#include "EditCinematicGUI.h"

#include <imgui.h>

namespace gui {

EditSelectionGUI::EditSelectionGUI()
  : IGUI()
{
  // Fill gui map, will choose correct GUI to render depending on selection type
  //_specificGuis[logic::AneditContext::SelectionType::Animator] = new EditAnimatorGUI();
  _specificGuis[logic::AneditContext::SelectionType::Node] = new EditNodeGUI();
  _specificGuis[logic::AneditContext::SelectionType::Material] = new EditMaterialGUI();
  _specificGuis[logic::AneditContext::SelectionType::Prefab] = new EditPrefabGUI();
  _specificGuis[logic::AneditContext::SelectionType::Cinematic] = new EditCinematicGUI();
}

EditSelectionGUI::~EditSelectionGUI()
{
  for (auto& item : _specificGuis) {
    delete item.second;
  }
}

void EditSelectionGUI::immediateDraw(logic::AneditContext* c)
{
  ImGui::Begin("Edit selection");
  _specificGuis[c->selectionType()]->immediateDraw(c);
  ImGui::End();
}

}