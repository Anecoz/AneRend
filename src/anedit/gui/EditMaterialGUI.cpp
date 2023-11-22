#include "EditMaterialGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <imgui.h>
#include <glm/glm.hpp>

namespace gui {

EditMaterialGUI::EditMaterialGUI()
  : IGUI()
{}

EditMaterialGUI::~EditMaterialGUI()
{}

void EditMaterialGUI::immediateDraw(logic::AneditContext* c)
{
  auto& id = c->selectedMaterial();

  ImGui::Begin("EditMaterial");

  glm::vec4 emissive;
  glm::vec3 baseColFac;

  bool changed = false;

  if (!id) {
    ImGui::BeginDisabled();
  }
  else {
    auto* mat = c->scene().getMaterial(id);

    emissive = mat->_emissive;
    baseColFac = mat->_baseColFactor;
  }

  ImGui::Text("Emissive");
  if (ImGui::ColorEdit4("##emissive", &emissive[0], ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR)) {
    changed = true;
  }

  ImGui::Separator();
  ImGui::Text("Base Color Factor");
  if (ImGui::ColorEdit3("##basecolfac", &baseColFac[0])) {
    changed = true;
  }

  if (!id) {
    ImGui::EndDisabled();
  }

  ImGui::End();

  if (changed && id) {
    auto matCopy = *c->scene().getMaterial(id);
    matCopy._emissive = emissive;
    matCopy._baseColFactor = baseColFac;
    c->scene().updateMaterial(std::move(matCopy));
  }
}

}