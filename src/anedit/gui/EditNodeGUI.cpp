#include "EditNodeGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include "EditTransformGUI.h"
#include "EditLightGUI.h"
#include "EditRenderableGUI.h"

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

  DRAW_COMP(Transform);
  DRAW_COMP(Renderable);
  DRAW_COMP(Light);

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

    ImGui::EndPopup();
  }
}

}