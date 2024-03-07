#include "EditMaterialGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>
#include <render/asset/AssetCollection.h>

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

  char name[100];
  name[0] = '\0';

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
    //auto* mat = c->scene().getMaterial(id);
    auto mat = c->assetCollection().getMaterialBlocking(id);

    emissive = mat._emissive;
    baseColFac = mat._baseColFactor;
    roughness = mat._roughnessFactor;
    metallic = mat._metallicFactor;
    strcpy_s(name, mat._name.c_str());

    metRoughTex = mat._metallicRoughnessTex;
    albedoTex = mat._albedoTex;
    normalTex = mat._normalTex;
    emissiveTex = mat._emissiveTex;
  }

  ImGui::Text("Name");
  if (ImGui::InputText("##matname", name, 100)) {
    changed = true;
  }

  ImGui::Separator();
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

  ImGui::Separator();
  ImGui::Text("Metallic roughness tex");

  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("texture_id")) {
      std::array<std::uint8_t, 16> arr{};
      std::memcpy(arr.data(), payload->Data, sizeof(util::Uuid));
      auto uuid = util::Uuid(arr);

      metRoughTex = uuid;

      changed = true;
    }

    ImGui::EndDragDropTarget();
  }

  if (metRoughTex) {
    ImGui::Image((ImTextureID)c->getImguiTexId(metRoughTex), texSize);
  }

  ImGui::Separator();
  ImGui::Text("Albedo tex");

  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("texture_id")) {
      std::array<std::uint8_t, 16> arr{};
      std::memcpy(arr.data(), payload->Data, sizeof(util::Uuid));
      auto uuid = util::Uuid(arr);

      albedoTex = uuid;

      changed = true;
    }

    ImGui::EndDragDropTarget();
  }

  if (albedoTex) {
    ImGui::Image((ImTextureID)c->getImguiTexId(albedoTex), texSize);
  }

  ImGui::Separator();
  ImGui::Text("Normal tex");

  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("texture_id")) {
      std::array<std::uint8_t, 16> arr{};
      std::memcpy(arr.data(), payload->Data, sizeof(util::Uuid));
      auto uuid = util::Uuid(arr);

      normalTex = uuid;

      changed = true;
    }

    ImGui::EndDragDropTarget();
  }

  if (normalTex) {
    ImGui::Image((ImTextureID)c->getImguiTexId(normalTex), texSize);
  }

  ImGui::Separator();
  ImGui::Text("Emissive tex");

  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("texture_id")) {
      std::array<std::uint8_t, 16> arr{};
      std::memcpy(arr.data(), payload->Data, sizeof(util::Uuid));
      auto uuid = util::Uuid(arr);

      emissiveTex = uuid;

      changed = true;
    }

    ImGui::EndDragDropTarget();
  }

  if (emissiveTex) {
    ImGui::Image((ImTextureID)c->getImguiTexId(emissiveTex), texSize);
  }

  if (!id) {
    ImGui::EndDisabled();
  }

  if (changed && id) {
    //auto matCopy = *c->scene().getMaterial(id);
    auto matCopy = c->assetCollection().getMaterialBlocking(id);
    matCopy._name = name;
    matCopy._emissive = emissive;
    matCopy._baseColFactor = baseColFac;
    matCopy._metallicFactor = metallic;
    matCopy._roughnessFactor = roughness;

    matCopy._metallicRoughnessTex = metRoughTex;
    matCopy._albedoTex = albedoTex;
    matCopy._normalTex = normalTex;
    matCopy._emissiveTex = emissiveTex;
    //c->scene().updateMaterial(std::move(matCopy));
    c->assetCollection().updateMaterial(std::move(matCopy));
  }
}

}