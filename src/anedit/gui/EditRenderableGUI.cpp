#include "EditRenderableGUI.h"

#include "../logic/AneditContext.h"
#include "../render/scene/Scene.h"

#include <glm/glm.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

namespace gui {

EditRenderableGUI::EditRenderableGUI()
  : IGUI()
{}

EditRenderableGUI::~EditRenderableGUI()
{}

void EditRenderableGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->selectedRenderable();
  glm::vec3 translation{ 0.0f };
  glm::vec3 scale{ 0.0f };

  ImGui::Begin("EditRenderable");

  {
    if (id == render::INVALID_ID) {
      ImGui::BeginDisabled();
    }
    else {
      auto oldTrans = c->scene().getRenderable(id)->_transform;

      // extract translation
      translation = oldTrans[3];

      // extract scale
      scale.x = glm::length(glm::vec3(oldTrans[0])); // Basis vector X
      scale.y = glm::length(glm::vec3(oldTrans[1])); // Basis vector Y
      scale.z = glm::length(glm::vec3(oldTrans[2])); // Basis vector Z
    }

    ImGui::Text("Translation");
    if (ImGui::InputFloat3("##translation", &translation[0], "%.3f", ImGuiInputTextFlags_EnterReturnsTrue) && id) {
      auto oldTrans = c->scene().getRenderable(id)->_transform;

      auto& t = oldTrans[3];
      t = glm::vec4(translation, 1.0f);

      c->scene().setRenderableTransform(id, oldTrans);
    }


    ImGui::Text("Scale");
    if (ImGui::InputFloat3("##scale", &scale[0], "%.3f", ImGuiInputTextFlags_EnterReturnsTrue) && id) {
      auto oldTrans = c->scene().getRenderable(id)->_transform;

      // normalize first
      auto& c0 = oldTrans[0];
      auto& c1 = oldTrans[1];
      auto& c2 = oldTrans[2];

      c0 = glm::normalize(c0);
      c1 = glm::normalize(c1);
      c2 = glm::normalize(c2);

      auto newTrans = glm::scale(oldTrans, scale);

      c->scene().setRenderableTransform(id, newTrans);
    }

    if (id == render::INVALID_ID) {
      ImGui::EndDisabled();
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
      ImGuizmo::OPERATION::TRANSLATE,
      ImGuizmo::MODE::WORLD,
      (float*)&m[0][0]);

    if (manipulated) {
      c->scene().setRenderableTransform(id, m);
      //printf("Manipulated!\n");
    }
  }

}

}