#include "EditNodeGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include "component/EditTransformGUI.h"
#include "component/EditLightGUI.h"
#include "component/EditRenderableGUI.h"
#include "component/EditAnimatorGUI.h"
#include "component/EditTerrainGUI.h"
#include "component/EditRigidBodyGUI.h"
#include "component/EditSphereColliderGUI.h"
#include "component/EditMeshColliderGUI.h"
#include "component/EditBoxColliderGUI.h"
#include "component/EditCharacterControllerGUI.h"

#include <imgui.h>

namespace gui {

namespace {

// Helper macro
#define DRAW_COMP(comp) \
if (c->scene().registry().hasComponent<component::comp>(id)) { \
  has##comp = true; \
  if (ImGui::CollapsingHeader(#comp, ImGuiTreeNodeFlags_DefaultOpen)) { \
    _componentGUIs[typeid(component::comp)]->immediateDraw(c); \
  } \
} \

}

EditNodeGUI::EditNodeGUI()
  : IGUI()
{
  _componentGUIs[typeid(component::Transform)] = new EditTransformGUI();
  _componentGUIs[typeid(component::Renderable)] = new EditRenderableGUI();
  _componentGUIs[typeid(component::Light)] = new EditLightGUI();
  _componentGUIs[typeid(component::Animator)] = new EditAnimatorGUI();
  _componentGUIs[typeid(component::Terrain)] = new EditTerrainGUI();
  _componentGUIs[typeid(component::RigidBody)] = new EditRigidBodyGUI();
  _componentGUIs[typeid(component::SphereCollider)] = new EditSphereColliderGUI();
  _componentGUIs[typeid(component::MeshCollider)] = new EditMeshColliderGUI();
  _componentGUIs[typeid(component::BoxCollider)] = new EditBoxColliderGUI();
  _componentGUIs[typeid(component::CharacterController)] = new EditCharacterControllerGUI();
}

EditNodeGUI::~EditNodeGUI()
{
  for (auto& pair : _componentGUIs) {
    delete pair.second;
  }
}

void EditNodeGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->getFirstSelection();

  if (!id || (c->selectionType() != logic::AneditContext::SelectionType::Node)) {
    return;
  }

  // Name field for the node itself
  char name[100];
  name[0] = '\0';
  const auto* node = c->scene().getNode(id);
  strcpy_s(name, node->_name.c_str());

  ImGui::Text("Name");
  if (ImGui::InputText("##nodename", name, 100) && id) {
    c->scene().setNodeName(id, name);
  }
  ImGui::Separator();

 
  bool hasTransform = false;
  bool hasRenderable = false;
  bool hasLight = false;
  bool hasAnimator = false;
  bool hasTerrain = false;
  bool hasRigidBody = false;
  bool hasSphereCollider = false;
  bool hasMeshCollider = false;
  bool hasBoxCollider = false;
  bool hasCharacterController = false;

  DRAW_COMP(Transform);
  DRAW_COMP(Renderable);
  DRAW_COMP(Light);
  DRAW_COMP(Animator);
  DRAW_COMP(Terrain);
  DRAW_COMP(RigidBody);
  DRAW_COMP(SphereCollider);
  DRAW_COMP(MeshCollider);
  DRAW_COMP(BoxCollider);
  DRAW_COMP(CharacterController);

  // Add new components
  if (ImGui::BeginPopupContextWindow()) {
    if (!hasRenderable) {
      if (ImGui::MenuItem("Add renderable...")) {
        // TODO: Need to select model, materials, skeleton etc. Probably only makes sense from a prefab?
        //       Or at least is easiest via prefab. Also of course doable without prefab
        //c->scene().registry().addComponent<component::Renderable>(id);
        //c->scene().registry().addComponent<component::Renderable>(id);
      }
    }
    if (!hasLight) {
      if (ImGui::MenuItem("Add light...")) {
        c->scene().registry().addComponent<component::Light>(id);
        c->scene().registry().patchComponent<component::Light>(id);
      }
    }
    if (!hasTerrain) {
      if (ImGui::MenuItem("Add terrain...")) {
        c->scene().registry().addComponent<component::Terrain>(id);
        c->scene().registry().patchComponent<component::Terrain>(id);
      }
    }
    if (!hasRigidBody) {
      if (ImGui::MenuItem("Add rigid body...")) {
        c->scene().registry().addComponent<component::RigidBody>(id);
        c->scene().registry().patchComponent<component::RigidBody>(id);
      }
    }
    if (!hasSphereCollider) {
      if (ImGui::MenuItem("Add sphere collider...")) {
        c->scene().registry().addComponent<component::SphereCollider>(id);
        c->scene().registry().patchComponent<component::SphereCollider>(id);
      }
    }
    if (!hasMeshCollider) {
      if (ImGui::MenuItem("Add mesh collider...")) {
        c->scene().registry().addComponent<component::MeshCollider>(id);
        c->scene().registry().patchComponent<component::MeshCollider>(id);
      }
    }
    if (!hasBoxCollider) {
      if (ImGui::MenuItem("Add box collider...")) {
        c->scene().registry().addComponent<component::BoxCollider>(id);
        c->scene().registry().patchComponent<component::BoxCollider>(id);
      }
    }
    if (!hasCharacterController) {
      if (ImGui::MenuItem("Add character controller...")) {
        c->scene().registry().addComponent<component::CharacterController>(id);
        c->scene().registry().patchComponent<component::CharacterController>(id);
      }
    }

    ImGui::EndPopup();
  }
}

}