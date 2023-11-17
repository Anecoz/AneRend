#include "SceneListGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <imgui.h>

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
    ImGui::Separator();

    for (const auto& r : renderables) {
      std::string label = std::string("Renderable ") + r._id.str();
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

}