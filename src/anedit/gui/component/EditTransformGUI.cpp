#include "EditTransformGUI.h"

#include "../../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <glm/glm.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

namespace gui {

EditTransformGUI::EditTransformGUI()
  : IGUI()
  , _currentGuizmoOp(ImGuizmo::OPERATION::TRANSLATE)
  , _currentGuizmoMode(ImGuizmo::MODE::WORLD)
{}

EditTransformGUI::~EditTransformGUI()
{}

void EditTransformGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->getFirstSelection();
  bool changed = false;
  glm::vec3 translation{ 0.0f };
  glm::vec3 scale{ 0.0f };
  glm::vec3 rot{ 0.0f };

  bool disabled = !id;

  if (disabled) {
    ImGui::BeginDisabled();
  }
  else {
    auto& transComp = c->scene().registry().getComponent<component::Transform>(id);

    auto oldTrans = transComp._localTransform;
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

  // Radio for changing between local and world relative
  if (ImGui::RadioButton("World", _currentGuizmoMode == ImGuizmo::WORLD)) {
    _currentGuizmoMode = ImGuizmo::WORLD;
  }

  ImGui::SameLine();
  if (ImGui::RadioButton("Local", _currentGuizmoMode == ImGuizmo::LOCAL)) {
    _currentGuizmoMode = ImGuizmo::LOCAL;
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

  if (disabled) {
    ImGui::EndDisabled();
  }
  else if (changed) {
    // recompose
    glm::mat4 m;
    ImGuizmo::RecomposeMatrixFromComponents(&translation[0], &rot[0], &scale[0], &m[0][0]);

    auto& transComp = c->scene().registry().getComponent<component::Transform>(id);
    transComp._localTransform = std::move(m);

    c->scene().registry().patchComponent<component::Transform>(id);
  }

  // Do gizmo editing
  if (!disabled) {
    auto& transComp = c->scene().registry().getComponent<component::Transform>(id);

    // Do we have a parent?
    glm::mat4 inverseParentGlobal{ 1.0f };
    auto* node = c->scene().getNode(id);
    if (node->_parent) {
      inverseParentGlobal = glm::inverse(
        c->scene().registry().getComponent<component::Transform>(node->_parent)._globalTransform
      );
    }

    auto& viewMatrix = c->camera().getCamMatrix();
    auto& projMatrix = c->camera().getProjection();

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    glm::mat4 m = transComp._globalTransform;

    bool manipulated = ImGuizmo::Manipulate(
      &viewMatrix[0][0],
      &projMatrix[0][0],
      (ImGuizmo::OPERATION)_currentGuizmoOp,
      (ImGuizmo::MODE)_currentGuizmoMode,
      (float*)&m[0][0]);

    if (manipulated) {
      m = inverseParentGlobal * m;
      transComp._localTransform = std::move(m);
      
      c->scene().registry().patchComponent<component::Transform>(id);
    }
  }
}

}