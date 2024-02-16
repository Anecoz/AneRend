#include "EditCharacterControllerGUI.h"

#include "../../logic/AneditContext.h"

#include <render/scene/Scene.h>

#include <imgui.h>

namespace gui {

EditCharacterControllerGUI::EditCharacterControllerGUI()
  : IGUI()
{}

EditCharacterControllerGUI::~EditCharacterControllerGUI()
{}

void EditCharacterControllerGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->getFirstSelection();

  if (!id) {
    return;
  }

  bool changed = false;
  auto& comp = c->scene().registry().getComponent<component::CharacterController>(id);

  if (ImGui::InputFloat("Mass", &comp._mass)) {
    comp._mass = glm::clamp(comp._mass, 0.5f, 5000.0f);
    changed = true;
  }

  if (changed) {
    c->scene().registry().patchComponent<component::CharacterController>(id);
  }
}

}