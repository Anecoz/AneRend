#include "EditRenderableGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <glm/glm.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

namespace gui {

EditRenderableGUI::EditRenderableGUI()
  : IGUI()
  , _currentGuizmoOp(ImGuizmo::OPERATION::TRANSLATE)
{}

EditRenderableGUI::~EditRenderableGUI()
{}

void EditRenderableGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->selectedRenderable();
  bool changed = false;
  glm::vec3 translation{ 0.0f };
  glm::vec3 scale{ 0.0f };
  glm::vec3 rot{ 0.0f };

  glm::vec3 tint{ 0.0f };

  glm::vec3 boundingSphereCenter{ 0.0f };
  float boundingSphereRadius = 0.0f;

  char name[32];

  ImGui::Begin("EditRenderable");

  {
    if (!id) {
      ImGui::BeginDisabled();
    }
    else {
      auto* rend = c->scene().getRenderable(id);

      auto oldTrans = rend->_transform;
      strcpy_s(name, rend->_name.c_str());
      tint = rend->_tint;
      boundingSphereCenter = glm::vec3(rend->_boundingSphere.x, rend->_boundingSphere.y, rend->_boundingSphere.z);
      boundingSphereRadius = rend->_boundingSphere.w;

      ImGuizmo::DecomposeMatrixToComponents(&oldTrans[0][0], &translation[0], &rot[0], &scale[0]);
    }

    // Radio buttons for choosing guizmo mode
    if (ImGui::RadioButton("Translate", _currentGuizmoOp == ImGuizmo::TRANSLATE)) {
      _currentGuizmoOp = ImGuizmo::TRANSLATE;
    }
      
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", _currentGuizmoOp == ImGuizmo::ROTATE)) {
      _currentGuizmoOp = ImGuizmo::ROTATE;
    }
      
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", _currentGuizmoOp == ImGuizmo::SCALE)) {
      _currentGuizmoOp = ImGuizmo::SCALE;
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
      c->scene().setRenderableTransform(id, m);
      c->scene().setRenderableName(id, name);
      c->scene().setRenderableTint(id, tint);
      c->scene().setRenderableBoundingSphere(id, glm::vec4(boundingSphereCenter, boundingSphereRadius));
    }
  }

  ImGui::End();

  // Experiment with ImGuizmo
  if (id) {
    auto m = c->scene().getRenderable(id)->_transform;
    auto& viewMatrix = c->camera().getCamMatrix();
    auto& projMatrix = c->camera().getProjection();

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    bool manipulated = ImGuizmo::Manipulate(
      &viewMatrix[0][0],
      &projMatrix[0][0],
      (ImGuizmo::OPERATION)_currentGuizmoOp,
      ImGuizmo::MODE::WORLD,
      (float*)&m[0][0]);

    if (manipulated) {
      c->scene().setRenderableTransform(id, m);
    }
  }

}

}