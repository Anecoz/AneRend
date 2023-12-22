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
  auto& id = c->getFirstSelection();

  glm::vec4 emissive;
  glm::vec3 baseColFac;
  float roughness = 1.0f;
  float metallic = 1.0f;

  bool changed = false;

  if (!id) {
    ImGui::BeginDisabled();
  }
  else {
    auto* mat = c->scene().getMaterial(id);

    emissive = mat->_emissive;
    baseColFac = mat->_baseColFactor;
    roughness = mat->_roughnessFactor;
    metallic = mat->_metallicFactor;
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

  ImGui::Separator();
  ImGui::Text("Roughness");
  if (ImGui::InputFloat("##roughness", &roughness)) {
    changed = true;
  }

  ImGui::Separator();
  ImGui::Text("Metallic");
  if (ImGui::InputFloat("##metallic", &metallic)) {
    changed = true;
  }

  if (!id) {
    ImGui::EndDisabled();
  }

  if (changed && id) {
    auto matCopy = *c->scene().getMaterial(id);
    matCopy._emissive = emissive;
    matCopy._baseColFactor = baseColFac;
    matCopy._metallicFactor = metallic;
    matCopy._roughnessFactor = roughness;
    c->scene().updateMaterial(std::move(matCopy));
  }
}

}