#include "SceneAssetGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <imgui.h>
#include <nfd.hpp>

namespace gui {

SceneAssetGUI::SceneAssetGUI()
  : IGUI()
{}

SceneAssetGUI::~SceneAssetGUI()
{}

void SceneAssetGUI::immediateDraw(logic::AneditContext* c)
{
  const int numPanels = 5;

  ImGui::Begin("Assets", NULL, ImGuiWindowFlags_MenuBar);
  ImVec2 size = ImVec2(ImGui::GetWindowWidth() / numPanels, 0);

  // Menu
  if (ImGui::BeginMenuBar()) {

    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load GLTF...")) {
        loadGLTFClicked(c);
      }

      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }

  // Materials
  {
    ImGui::BeginChild("Materials", size, true);
    ImGui::Text("Materials");
    ImGui::Separator();

    const auto& mats = c->scene().getMaterials();
    for (const auto& mat : mats) {
      std::string label = std::string("Mat ") + (mat._name.empty() ? mat._id.str() : mat._name);
      if (ImGui::Selectable(label.c_str(), _selectedMaterial == mat._id)) {
        _selectedMaterial = mat._id;
        printf("Selected mat %s\n", _selectedMaterial.str().c_str());
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
      std::string label = std::string("Model ") + (model._name.empty() ? model._id.str() : model._name);
      if (ImGui::Selectable(label.c_str(), _selectedModel == model._id)) {
        _selectedModel = model._id;
        printf("Selected model %s\n", _selectedModel.str().c_str());
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
      std::string label = std::string("Animation ") + (i._name.empty()? i._id.str() : i._name);
      if (ImGui::Selectable(label.c_str(), _selectedAnimation == i._id)) {
        _selectedAnimation = i._id;
        printf("Selected animation %s\n", _selectedAnimation.str().c_str());
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
      std::string label = std::string("Skeleton ") + (i._name.empty() ? i._id.str() : i._name);
      if (ImGui::Selectable(label.c_str(), _selectedSkeleton == i._id)) {
        _selectedSkeleton = i._id;
        printf("Selected skeleton %s\n", _selectedSkeleton.str().c_str());
      }
    }

    ImGui::EndChild();
  }

  ImGui::SameLine();

  // Prefabs
  {
    ImGui::BeginChild("Prefabs", size, true);
    ImGui::Text("Prefabs");
    ImGui::Separator();

    const auto& v = c->scene().getPrefabs();
    for (const auto& i : v) {
      std::string label = std::string("Prefab ") + (i._name.empty() ? i._id.str() : i._name);
      if (ImGui::Selectable(label.c_str(), _selectedPrefab == i._id)) {
        _selectedPrefab = i._id;
        printf("Selected prefab %s\n", _selectedPrefab.str().c_str());
      }
    }

    ImGui::EndChild();
  }

  ImGui::End();
}

void SceneAssetGUI::loadGLTFClicked(logic::AneditContext* c)
{
  // Open a file dialog and ask context to load GLTF
  NFD::UniquePath outPath;
  nfdfilteritem_t filterItem[1] = { {"GLTF", "gltf,glb"} };

  auto result = NFD::OpenDialog(outPath, filterItem, 1);
  if (result == NFD_OKAY) {
    c->startLoadGLTF(outPath.get());
  }
}

}