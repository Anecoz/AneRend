#include "EditSelectionGUI.h"

#include "EditAnimatorGUI.h"
#include "EditNodeGUI.h"
#include "EditMaterialGUI.h"
#include "EditPrefabGUI.h"

#include <imgui.h>

namespace gui {

namespace {

std::string selectionTypeToString(logic::AneditContext::SelectionType type)
{
  switch (type) {
  case logic::AneditContext::SelectionType::Animator:
    return "Animator";
  case logic::AneditContext::SelectionType::Node:
    return "Node";
  case logic::AneditContext::SelectionType::Material:
    return "Material";
  case logic::AneditContext::SelectionType::Prefab:
    return "Prefab";
  }

  return "";
}

}

EditSelectionGUI::EditSelectionGUI()
  : IGUI()
{
  // Fill gui map, will choose correct GUI to render depending on selection type
  _specificGuis[logic::AneditContext::SelectionType::Animator] = new EditAnimatorGUI();
  _specificGuis[logic::AneditContext::SelectionType::Node] = new EditNodeGUI();
  _specificGuis[logic::AneditContext::SelectionType::Material] = new EditMaterialGUI();
  _specificGuis[logic::AneditContext::SelectionType::Prefab] = new EditPrefabGUI();
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