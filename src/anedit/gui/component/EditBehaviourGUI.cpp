#include "EditBehaviourGUI.h"

#include "../../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <imgui.h>

namespace gui {

EditBehaviourGUI::EditBehaviourGUI()
  : IGUI()
{}

EditBehaviourGUI::~EditBehaviourGUI()
{}

void EditBehaviourGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->getFirstSelection();

  if (!id) {
    return;
  }

  bool changed = false;
  auto& behComp = c->scene().registry().getComponent<component::Behaviour>(id);

  // Name
  char name[100];
  name[0] = '\0';
  strcpy_s(name, behComp._name.c_str());

  ImGui::Text("Name");
  if (ImGui::InputText("##behname", name, 100) && id) {
    behComp._name = name;
    changed = true;
  }

  if (changed) {
    c->scene().registry().patchComponent<component::Behaviour>(id);
  }
}

}