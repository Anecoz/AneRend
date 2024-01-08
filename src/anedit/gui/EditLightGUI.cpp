#include "EditLightGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <glm/glm.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

namespace gui {

EditLightGUI::EditLightGUI()
  : IGUI()
{}

EditLightGUI::~EditLightGUI()
{}

void EditLightGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->getFirstSelection();
  bool changed = false;
  glm::vec3 pos{ 0.0f };
  glm::vec3 color{ 0.0f };
  float range = 0.0f;
  bool enabled = false;
  bool shadowCaster = false;

  char name[32];
  name[0] = '\0';

  //ImGui::Begin("EditLight");

  {
    if (!id) {
      ImGui::BeginDisabled();
    }
    else {
      /*auto* light = c->scene().getLight(id);

      strcpy_s(name, light->_name.c_str());
      pos = light->_pos;
      color = light->_color;
      range = light->_range;
      enabled = light->_enabled;
      shadowCaster = light->_shadowCaster;*/
    }

    // Input for pos
    ImGui::Text("Pos");
    if (ImGui::InputFloat3("##pos", &pos[0], "%.3f", ImGuiInputTextFlags_EnterReturnsTrue) && id) {
      changed = true;
    }

    // range
    ImGui::Separator();
    ImGui::Text("Range");
    if (ImGui::InputFloat("##range", &range)) {
      changed = true;
    }

    // Color
    ImGui::Separator();
    ImGui::Text("Color");
    if (ImGui::ColorEdit3("##color", &color[0])) {
      changed = true;
    }

    // Name
    ImGui::Separator();
    ImGui::Text("Name");
    if (ImGui::InputText("##name", name, 32) && id) {
      changed = true;
    }

    // enabled flag
    ImGui::Separator();
    ImGui::Text("Enabled");
    if (ImGui::Checkbox("##enabled", &enabled)) {
      changed = true;
    }

    // shadowcaster flag
    ImGui::Separator();
    ImGui::Text("Shadow caster");
    if (ImGui::Checkbox("##shadowcaster", &shadowCaster)) {
      changed = true;
    }

    if (!id) {
      ImGui::EndDisabled();
    }
    else if (changed) {
      /*auto oldLight = *c->scene().getLight(id);
      oldLight._name = std::string(name);
      oldLight._color = color;
      oldLight._pos = pos;
      oldLight._enabled = enabled;
      oldLight._range = range;
      oldLight._shadowCaster = shadowCaster;

      c->scene().updateLight(std::move(oldLight));*/
    }
  }

  //ImGui::End();

  // Do gizmo (transform) editing with ImGuizmo
  if (id) {
    /*auto* light = c->scene().getLight(id);

    glm::mat4 m = glm::translate(glm::mat4(1.0f), light->_pos);

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
      auto copy = *light;
      glm::vec3 t = m[3];
      copy._pos = t;

      c->scene().updateLight(std::move(copy));
    }*/
  }

}

}