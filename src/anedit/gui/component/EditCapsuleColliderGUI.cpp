#include "EditCapsuleColliderGUI.h"

#include "../../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <imgui.h>

namespace gui {

EditCapsuleColliderGUI::EditCapsuleColliderGUI()
  : IGUI()
{}

EditCapsuleColliderGUI::~EditCapsuleColliderGUI()
{}

void EditCapsuleColliderGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->getFirstSelection();

  if (!id) {
    return;
  }

  bool changed = false;
  auto& capsuleComp = c->scene().registry().getComponent<component::CapsuleCollider>(id);

  // Halfheight
  if (ImGui::InputFloat("Half height", &capsuleComp._halfHeight)) changed = true;
  if (ImGui::InputFloat("Radius", &capsuleComp._radius)) changed = true;

  if (changed) {
    c->scene().registry().patchComponent<component::CapsuleCollider>(id);
  }
}

}