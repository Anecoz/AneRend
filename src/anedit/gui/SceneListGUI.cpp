#include "SceneListGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <imgui.h>
#include <nfd.hpp>

namespace gui {

SceneListGUI::SceneListGUI()
  : IGUI()
{}

SceneListGUI::~SceneListGUI()
{}

void SceneListGUI::immediateDraw(logic::AneditContext* c)
{
  // Draw list of current renderables, animators, lights, particle emitters etc
  const auto& renderables = c->scene().getRenderables();

  ImGui::Begin("Scene list");

  {
    ImGui::BeginChild("Renderables", ImVec2(0, 0), true);
    ImGui::Text("Renderables");

    ImGui::SameLine();
    if (ImGui::Button("Add...")) {
      ImGui::OpenPopup("Add prefab");
    }
    addFromPrefab(c); // this has to run every frame but only executes if the button above was pressed

    //ImGui::SameLine();
    if (ImGui::Button("Save scene as...")) {
      saveSceneAsClicked(c);
    }
    ImGui::SameLine();
    if (ImGui::Button("Save scene")) {
      saveSceneClicked(c);
    }

    //ImGui::SameLine();
    if (ImGui::Button("Load scene...")) {
      loadSceneClicked(c);
    }

    ImGui::Separator();

    for (const auto& r : renderables) {
      std::string label = std::string("Renderable ") + (r._name.empty()? r._id.str() : r._name);
      label += "##" + r._id.str();
      if (ImGui::Selectable(label.c_str(), _selectedRenderable == r._id)) {
        _selectedRenderable = r._id;
        c->selectedRenderable() = _selectedRenderable;
        printf("Selected rend %s\n", _selectedRenderable.str().c_str());
      }
    }

    ImGui::EndChild();
  }

  ImGui::End();
}

void SceneListGUI::addFromPrefab(logic::AneditContext* c)
{
  if (ImGui::BeginPopupModal("Add prefab")) {
    
    // List all prefabs as selectable
    auto& prefabs = c->scene().getPrefabs();
    for (const auto& p : prefabs) {
      std::string label = std::string("Prefab ") + (p._name.empty()? p._id.str() : p._name);
      label += "##" + p._id.str();
      if (ImGui::Selectable(label.c_str(), p._id == _selectedPrefab, ImGuiSelectableFlags_DontClosePopups)) {
        _selectedPrefab = p._id;
      }
    }

    if (ImGui::Button("OK")) {

      // Create a renderable from prefab
      if (_selectedPrefab) {
        auto* p = c->scene().getPrefab(_selectedPrefab);
        render::asset::Renderable rend{};
        rend._materials = p->_materials;
        rend._model = p->_model;
        rend._skeleton = p->_skeleton;

        rend._boundingSphere = glm::vec4(0.0f, 0.0f, 0.0f, 20.0f);
        rend._visible = true;
        rend._transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f));

        c->scene().addRenderable(std::move(rend));
      }

      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void SceneListGUI::loadSceneClicked(logic::AneditContext* c)
{
  // Open a file dialog and choose where to load from
  NFD::UniquePath outPath;

  nfdfilteritem_t filterItem[1] = { {"Anedit scene", "anescene"} };

  auto result = NFD::OpenDialog(outPath, filterItem, 1);
  if (result == NFD_OKAY) {
    c->setScenePath(outPath.get());
    c->loadSceneFrom(outPath.get());

    printf("Loading scene from %s\n", outPath.get());
  }
  else {
    printf("Could not load scene!\n");
  }
}

void SceneListGUI::saveSceneClicked(logic::AneditContext* c)
{
  c->serializeScene();
}

void SceneListGUI::saveSceneAsClicked(logic::AneditContext* c)
{
  // Open a file dialog and choose where to serialize the scene
  NFD::UniquePath outPath;

  nfdfilteritem_t filterItem[1] = { {"Anedit scene", "anescene"} };

  auto result = NFD::SaveDialog(outPath, filterItem, 1, nullptr, "scene.anescene");
  if (result == NFD_OKAY) {
    c->setScenePath(outPath.get());
    c->serializeScene();

    printf("Wrote scene to %s\n", outPath.get());
  }
  else {
    printf("Could not write scene!\n");
  }
}

}