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
  auto id = c->getFirstSelection();

  glm::vec4 emissive;
  glm::vec3 baseColFac;
  float roughness = 1.0f;
  float metallic = 1.0f;

  // Textures
  util::Uuid metRoughTex;
  util::Uuid albedoTex;
  util::Uuid normalTex;
  util::Uuid emissiveTex;

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

    metRoughTex = mat->_metallicRoughnessTex;
    albedoTex = mat->_albedoTex;
    normalTex = mat->_normalTex;
    emissiveTex = mat->_emissiveTex;
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

  float texFactor = 0.75f;
  auto maxRegion = ImGui::GetWindowContentRegionMax();
  ImVec2 texSize{ texFactor * maxRegion.x , texFactor * maxRegion.x };

  if (metRoughTex) {
    ImGui::Separator();
    ImGui::Text("Metallic roughness tex");
    ImGui::Image((ImTextureID)c->getImguiTexId(metRoughTex), texSize);
  }

  if (albedoTex) {
    ImGui::Separator();
    ImGui::Text("Albedo tex");
    ImGui::Image((ImTextureID)c->getImguiTexId(albedoTex), texSize);
  }

  if (normalTex) {
    ImGui::Separator();
    ImGui::Text("Normal tex");
    ImGui::Image((ImTextureID)c->getImguiTexId(normalTex), texSize);
  }

  if (emissiveTex) {
    ImGui::Separator();
    ImGui::Text("Emissive tex");
    ImGui::Image((ImTextureID)c->getImguiTexId(emissiveTex), texSize);
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