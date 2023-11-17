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

  // Materials
  {
    ImGui::BeginChild("Renderables", ImVec2(0, 0), true);
    ImGui::Text("Renderables");
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

}