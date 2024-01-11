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
  glm::vec3 color{ 0.0f };
  float range = 0.0f;
  bool enabled = false;
  bool shadowCaster = false;

  char name[32];
  name[0] = '\0';

  {
    if (!id) {
      ImGui::BeginDisabled();
    }
    else {
      auto& lightComp = c->scene().registry().getComponent<component::Light>(id);

      strcpy_s(name, lightComp._name.c_str());
      color = lightComp._color;
      range = lightComp._range;
      enabled = lightComp._enabled;
      shadowCaster = lightComp._shadowCaster;
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
      auto& lightComp = c->scene().registry().getComponent<component::Light>(id);

      lightComp._name = std::string(name);
      lightComp._color = color;
      lightComp._enabled = enabled;
      lightComp._range = range;
      lightComp._shadowCaster = shadowCaster;

      c->scene().registry().patchComponent<component::Light>(id);
    }
  }
}

}