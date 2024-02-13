#include "EditBoxColliderGUI.h"

#include "../../logic/AneditContext.h"

#include <render/scene/Scene.h>

#include <imgui.h>

namespace gui {

EditBoxColliderGUI::EditBoxColliderGUI()
  : IGUI()
{}

EditBoxColliderGUI::~EditBoxColliderGUI()
{}

void EditBoxColliderGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->getFirstSelection();

  if (!id) {
    return;
  }

  bool changed = false;
  auto& boxColliderComp = c->scene().registry().getComponent<component::BoxCollider>(id);
  
  // Half extent parameter
  if (ImGui::InputFloat3("Half Extent", &boxColliderComp._halfExtent[0])) {

    boxColliderComp._halfExtent = glm::clamp(boxColliderComp._halfExtent, glm::vec3(0.051f), glm::vec3(1000.0f));

    changed = true;
  }

  if (changed) {
    c->scene().registry().patchComponent<component::BoxCollider>(id);
  }
}

}
