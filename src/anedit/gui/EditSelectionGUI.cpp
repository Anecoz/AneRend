#include "EditSelectionGUI.h"

#include "EditAnimatorGUI.h"
#include "EditRenderableGUI.h"
#include "EditLightGUI.h"
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
  case logic::AneditContext::SelectionType::Renderable:
    return "Renderable";
  case logic::AneditContext::SelectionType::Light:
    return "Light";
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
  _specificGuis[logic::AneditContext::SelectionType::Renderable] = new EditRenderableGUI();
  _specificGuis[logic::AneditContext::SelectionType::Light] = new EditLightGUI();
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
  std::string label = "Edit selection";// +selectionTypeToString(c->selectionType());
  ImGui::Begin(label.c_str());
  _specificGuis[c->selectionType()]->immediateDraw(c);
  ImGui::End();
}

}