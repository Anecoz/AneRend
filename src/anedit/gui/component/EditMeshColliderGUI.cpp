#include "EditMeshColliderGUI.h"

#include "../../logic/AneditContext.h"

#include <render/scene/Scene.h>

#include <imgui.h>

namespace gui {

EditMeshColliderGUI::EditMeshColliderGUI()
  : IGUI()
{}

EditMeshColliderGUI::~EditMeshColliderGUI()
{}

void EditMeshColliderGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->getFirstSelection();

  if (!id) {
    return;
  }

  ImGui::Text("No params");  
}

}
