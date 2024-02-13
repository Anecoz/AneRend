#include "EditRigidBodyGUI.h"

#include "../../logic/AneditContext.h"

#include <render/scene/Scene.h>

#include <imgui.h>

namespace gui {

namespace {

static std::vector<component::RigidBody::MotionType> g_MotionTypes = {
  component::RigidBody::MotionType::Dynamic,
  component::RigidBody::MotionType::Static,
  component::RigidBody::MotionType::Kinematic
};

std::string motionTypeToString(component::RigidBody::MotionType type)
{
  switch (type) {
  case component::RigidBody::MotionType::Dynamic:
    return "Dynamic";
  case component::RigidBody::MotionType::Static:
    return "Static";
  case component::RigidBody::MotionType::Kinematic:
    return "Kinematic";
  }

  return "";
}

}

EditRigidBodyGUI::EditRigidBodyGUI()
  : IGUI()
{}

EditRigidBodyGUI::~EditRigidBodyGUI()
{}

void EditRigidBodyGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->getFirstSelection();

  if (!id) {
    return;
  }

  bool changed = false;
  auto& rigidBodyComp = c->scene().registry().getComponent<component::RigidBody>(id);
  
  // Motion type combo box
  auto str = motionTypeToString(rigidBodyComp._motionType);
  if (ImGui::BeginCombo("Motion type", str.c_str())) {
    for (int i = 0; i < g_MotionTypes.size(); ++i) {
      auto motionStr = motionTypeToString(g_MotionTypes[i]);
      bool isSelected = motionStr == str;

      if (ImGui::Selectable(motionStr.c_str(), isSelected)) {
        rigidBodyComp._motionType = g_MotionTypes[i];
        changed = true;
      }

      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }

    ImGui::EndCombo();
  }

  // Bunch of parameters
  if (ImGui::InputFloat("Friction", &rigidBodyComp._friction)) changed = true;
  if (ImGui::InputFloat("Restitution", &rigidBodyComp._restitution)) changed = true;
  if (ImGui::InputFloat("Linear Damping", &rigidBodyComp._linearDamping)) changed = true;
  if (ImGui::InputFloat("Angular Damping", &rigidBodyComp._angularDamping)) changed = true;
  if (ImGui::InputFloat("Gravity Factor", &rigidBodyComp._gravityFactor)) changed = true;
  if (ImGui::InputFloat("Mass", &rigidBodyComp._mass)) changed = true;

  if (changed) {
    c->scene().registry().patchComponent<component::RigidBody>(id);
  }
}

}