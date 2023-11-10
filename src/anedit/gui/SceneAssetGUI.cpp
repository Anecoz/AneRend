#include "SceneAssetGUI.h"

#include "../logic/AneditContext.h"
#include "../render/scene/Scene.h"

#include <imgui.h>

namespace gui {

SceneAssetGUI::SceneAssetGUI()
  : IGUI()
{}

SceneAssetGUI::~SceneAssetGUI()
{}

void SceneAssetGUI::immediateDraw(logic::AneditContext* c)
{
  const int numPanels = 4;

  ImGui::Begin("Assets");
  ImVec2 size = ImVec2(ImGui::GetWindowWidth() / numPanels, 0);

  // Materials
  {
    ImGui::BeginChild("Materials", size, true);
    ImGui::Text("Materials");
    ImGui::Separator();

    const auto& mats = c->scene().getMaterials();
    for (const auto& mat : mats) {
      std::string label = std::string("Mat ") + std::to_string(mat._id);
      if (ImGui::Selectable(label.c_str(), _selectedMaterial == mat._id)) {
        _selectedMaterial = mat._id;
        printf("Selected mat %u\n", _selectedMaterial);
      }
    }

    ImGui::EndChild();
  }

  ImGui::SameLine();

  // Models
  {
    ImGui::BeginChild("Models", size, true);
    ImGui::Text("Models");
    ImGui::Separator();

    const auto& models = c->scene().getModels();
    for (const auto& model : models) {
      std::string label = std::string("Model ") + std::to_string(model._id);
      if (ImGui::Selectable(label.c_str(), _selectedModel == model._id)) {
        _selectedModel = model._id;
        printf("Selected model %u\n", _selectedModel);
      }
    }

    ImGui::EndChild();
  }

  ImGui::SameLine();

  // Animations
  {
    ImGui::BeginChild("Animations", size, true);
    ImGui::Text("Animations");
    ImGui::Separator();

    const auto& v = c->scene().getAnimations();
    for (const auto& i : v) {
      std::string label = std::string("Animation ") + std::to_string(i._id);
      if (ImGui::Selectable(label.c_str(), _selectedAnimation == i._id)) {
        _selectedAnimation = i._id;
        printf("Selected animation %u\n", _selectedAnimation);
      }
    }

    ImGui::EndChild();
  }

  ImGui::SameLine();

  // Skeletons
  {
    ImGui::BeginChild("Skeletons", size, true);
    ImGui::Text("Skeletons");
    ImGui::Separator();

    const auto& v = c->scene().getSkeletons();
    for (const auto& i : v) {
      std::string label = std::string("Skeleton ") + std::to_string(i._id);
      if (ImGui::Selectable(label.c_str(), _selectedSkeleton == i._id)) {
        _selectedSkeleton = i._id;
        printf("Selected skeleton %u\n", _selectedSkeleton);
      }
    }

    ImGui::EndChild();
  }

  ImGui::End();
}

}