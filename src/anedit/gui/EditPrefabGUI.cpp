#include "EditPrefabGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>
#include <render/asset/AssetCollection.h>

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

  bool changed = false;

  // Name
  char name[100];
  name[0] = '\0';
  //auto* prefab = c->scene().getPrefab(id);
  auto prefab = c->assetCollection().getPrefabBlocking(id);
  strcpy_s(name, prefab._name.c_str());
  ImGui::Text("Name");
  if (ImGui::InputText("##prefabname", name, 100)) {
    changed = true;
  }

  // Transform
  auto trans = prefab._comps._trans._localTransform;
  glm::vec3 translation{ 0.0f };
  glm::vec3 scale{ 0.0f };
  glm::vec3 rot{ 0.0f };
  ImGuizmo::DecomposeMatrixToComponents(&trans[0][0], &translation[0], &rot[0], &scale[0]);

  // Inputs for transform
  ImGui::Text("Translation");
  if (ImGui::InputFloat3("##prefabtranslation", &translation[0], "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
    changed = true;
  }

  ImGui::Text("Scale");
  if (ImGui::InputFloat3("##prefabscale", &scale[0], "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
    changed = true;
  }

  ImGui::Text("Rotation");
  if (ImGui::InputFloat3("##rprefabot", &rot[0], "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
    changed = true;
  }

  if (changed) {
    glm::mat4 m;
    ImGuizmo::RecomposeMatrixFromComponents(&translation[0], &rot[0], &scale[0], &m[0][0]);

    //auto copy = *prefab;
    prefab._name = name;
    prefab._comps._trans._localTransform = m;
    //c->scene().updatePrefab(std::move(copy));
    c->assetCollection().updatePrefab(std::move(prefab));
  }
}

}