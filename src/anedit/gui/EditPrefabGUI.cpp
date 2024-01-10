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
  bool changed = false;
  glm::vec3 translation{ 0.0f };
  glm::vec3 scale{ 0.0f };
  glm::vec3 rot{ 0.0f };

  glm::vec3 tint{ 0.0f };

  glm::vec3 boundingSphereCenter{ 0.0f };
  float boundingSphereRadius = 0.0f;

  char name[32];
  name[0] = '\0';

  {
    if (!id) {
      ImGui::BeginDisabled();
    }
    else {
#if 0
      auto* prefab = c->scene().getPrefab(id);

      auto oldTrans = prefab->_transform;
      strcpy_s(name, prefab->_name.c_str());
      tint = prefab->_tint;
      boundingSphereCenter = glm::vec3(prefab->_boundingSphere.x, prefab->_boundingSphere.y, prefab->_boundingSphere.z);
      boundingSphereRadius = prefab->_boundingSphere.w;

      ImGuizmo::DecomposeMatrixToComponents(&oldTrans[0][0], &translation[0], &rot[0], &scale[0]);
#endif
    }

    // Inputs for transform
    ImGui::Text("Translation");
    if (ImGui::InputFloat3("##translation", &translation[0], "%.3f", ImGuiInputTextFlags_EnterReturnsTrue) && id) {
      changed = true;
    }

    ImGui::Text("Scale");
    if (ImGui::InputFloat3("##scale", &scale[0], "%.3f", ImGuiInputTextFlags_EnterReturnsTrue) && id) {
      changed = true;
    }

    ImGui::Text("Rotation");
    if (ImGui::InputFloat3("##rot", &rot[0], "%.3f", ImGuiInputTextFlags_EnterReturnsTrue) && id) {
      changed = true;
    }

    // Name
    ImGui::Separator();
    ImGui::Text("Name");
    if (ImGui::InputText("##name", name, 32) && id) {
      changed = true;
    }

    // Tint
    ImGui::Separator();
    ImGui::Text("Tint");
    if (ImGui::ColorEdit3("##tint", &tint[0]) && id) {
      changed = true;
    }

    // Bounding sphere
    ImGui::Separator();
    ImGui::Text("Bounding Sphere");
    if (ImGui::InputFloat3("##boundingSphereCenter", &boundingSphereCenter[0], "%.3f", ImGuiInputTextFlags_EnterReturnsTrue) && id) {
      changed = true;
    }

    if (ImGui::InputFloat("##boundingSphereRadius", &boundingSphereRadius) && id) {
      changed = true;
    }

    if (!id) {
      ImGui::EndDisabled();
    }
    else if (changed) {
      // recompose
      glm::mat4 m;
      ImGuizmo::RecomposeMatrixFromComponents(&translation[0], &rot[0], &scale[0], &m[0][0]);

      auto prefabCopy = *c->scene().getPrefab(id);

#if 0
      prefabCopy._boundingSphere = glm::vec4(boundingSphereCenter, boundingSphereRadius);
      prefabCopy._name = name;
      prefabCopy._tint = tint;
      prefabCopy._transform = m;
#endif

      c->scene().updatePrefab(std::move(prefabCopy));
    }
  }
}

}