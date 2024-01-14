#include "EditPrefabGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <glm/glm.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

namespace gui {

EditPrefabGUI::EditPrefabGUI()
  : IGUI()
{}

EditPrefabGUI::~EditPrefabGUI()
{}

void EditPrefabGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->getFirstSelection();
  if (!id || (c->selectionType() != logic::AneditContext::SelectionType::Prefab)) {
    return;
  }

  // Name
  char name[100];
  name[0] = '\0';
  auto* prefab = c->scene().getPrefab(id);
  strcpy_s(name, prefab->_name.c_str());
  ImGui::Text("Name");
  if (ImGui::InputText("##prefabname", name, 100)) {
    auto copy = *prefab;
    copy._name = name;
    c->scene().updatePrefab(std::move(copy));
  }
}

}