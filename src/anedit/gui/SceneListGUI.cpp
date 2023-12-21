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
  const auto& animators = c->scene().getAnimators();
  const auto& lights = c->scene().getLights();

  ImGui::Begin("Scene list", NULL, ImGuiWindowFlags_MenuBar);

  // Menu
  std::string menuAction;
  if (ImGui::BeginMenuBar()) {

    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Add renderable...")) {
        menuAction = "AddRenderable";
      }
      if (ImGui::MenuItem("Add animator...")) {
        addAnimatorClicked(c);
      }
      if (ImGui::MenuItem("Add light...")) {
        addLightClicked(c);
      }
      if (ImGui::MenuItem("Save scene as...")) {
        saveSceneAsClicked(c);
      }
      if (ImGui::MenuItem("Save scene")) {
        saveSceneClicked(c);
      }
      if (ImGui::MenuItem("Load scene...")) {
        loadSceneClicked(c);
      }

      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }

  // renderables
  {
    ImGui::Text("Renderables");

    if (menuAction == "AddRenderable") {
      ImGui::OpenPopup("Add from prefab");
    }
    addFromPrefab(c); // this has to run every frame but only executes if the button above was pressed

    ImGui::Separator();

    for (const auto& r : renderables) {
      std::string label = std::string("Renderable ") + (r._name.empty()? r._id.str() : r._name);
      label += "##" + r._id.str();
      if (ImGui::Selectable(label.c_str(), _selectedRenderable == r._id)) {
        _selectedRenderable = r._id;
        c->selectedRenderable() = _selectedRenderable;
        c->latestSelection() = _selectedRenderable;
      }
    }
  }

  ImGui::Separator();

  // animators
  {
    ImGui::Text("Animators");

    ImGui::Separator();

    for (const auto& a : animators) {
      std::string label = std::string("Animator ") + (a._name.empty() ? a._id.str() : a._name);
      label += "##" + a._id.str();
      if (ImGui::Selectable(label.c_str(), _selectedAnimator == a._id)) {
        _selectedAnimator = a._id;
        c->selectedAnimator() = _selectedAnimator;
      }
    }
  }

  ImGui::Separator();

  // lights
  {
    ImGui::Text("Lights");

    ImGui::Separator();

    for (const auto& a : lights) {
      std::string label = std::string("Light ") + (a._name.empty() ? a._id.str() : a._name);
      label += "##" + a._id.str();
      if (ImGui::Selectable(label.c_str(), _selectedLight == a._id)) {
        _selectedLight = a._id;
        c->selectedLight() = _selectedLight;
        c->latestSelection() = _selectedLight;
      }
    }
  }

  ImGui::End();
}

void SceneListGUI::addFromPrefab(logic::AneditContext* c)
{
  /*if (ImGui::BeginPopupModal("Add from prefab")) {
    
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

        rend._boundingSphere = p->_boundingSphere;
        rend._visible = true;
        rend._transform = p->_transform;
        rend._tint = p->_tint;

        c->scene().addRenderable(std::move(rend));
      }

      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }*/
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

void SceneListGUI::addAnimatorClicked(logic::AneditContext* c)
{
  // Just add a default animator with no skele and anim
  render::asset::Animator animator{};
  animator._name = "New animator";
  animator._state = render::asset::Animator::State::Stopped;
  c->scene().addAnimator(std::move(animator));
}

void SceneListGUI::addLightClicked(logic::AneditContext* c)
{
  // Add a default light
  render::asset::Light light{};
  light._name = "New light";
  c->scene().addLight(std::move(light));
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