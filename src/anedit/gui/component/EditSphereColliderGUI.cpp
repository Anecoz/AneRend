#include "EditSphereColliderGUI.h"

#include "../../logic/AneditContext.h"

#include <render/scene/Scene.h>

#include <imgui.h>

namespace gui {

EditSphereColliderGUI::EditSphereColliderGUI()
  : IGUI()
{}

EditSphereColliderGUI::~EditSphereColliderGUI()
{}

void EditSphereColliderGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->getFirstSelection();

  if (!id) {
    return;
  }

  bool changed = false;
  auto& sphereColliderComp = c->scene().registry().getComponent<component::SphereCollider>(id);

  // Radius parameter
  if (ImGui::InputFloat("Radius", &sphereColliderComp._radius)) changed = true;

  if (changed) {
    sphereColliderComp._radius = glm::clamp(sphereColliderComp._radius, 0.051f, 1000.0f);

    c->scene().registry().patchComponent<component::SphereCollider>(id);
  }
}

}
