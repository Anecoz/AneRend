#include "SceneListGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <imgui.h>
#include <nfd.hpp>

namespace gui {

SceneListGUI::SceneListGUI()
  : IGUI()
{}

SceneListGUI::~SceneListGUI()
{}

void SceneListGUI::immediateDraw(logic::AneditContext* c)
{
  // Draw list of current nodes
  const auto& animators = c->scene().getAnimators();
  const auto& nodes = c->scene().getNodes();

  ImGui::Begin("Scene list", NULL, ImGuiWindowFlags_MenuBar);

  // Menu
  std::string menuAction;
  if (ImGui::BeginMenuBar()) {

    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Add renderable...")) {
        menuAction = "AddRenderable";
      }
      if (ImGui::MenuItem("Add animator...")) {
        addAnimatorClicked(c);
      }
      if (ImGui::MenuItem("Add light...")) {
        addLightClicked(c);
      }
      if (ImGui::MenuItem("Save scene as...")) {
        saveSceneAsClicked(c);
      }
      if (ImGui::MenuItem("Save scene")) {
        saveSceneClicked(c);
      }
      if (ImGui::MenuItem("Load scene...")) {
        loadSceneClicked(c);
      }

      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }

  // nodes
  {
    ImGui::Text("Nodes");

    if (menuAction == "AddRenderable") {
      ImGui::OpenPopup("Add from prefab");
    }
    addFromPrefab(c); // this has to run every frame but only executes if the button above was pressed

    ImGui::Separator();

    for (const auto& n : nodes) {
      // Only do top-level nodes, children will be rendered when their parents are rendered
      if (n._parent) {
        continue;
      }

      auto id = n._id;
      renderNodeTree(id, c);
    }
  }

  ImGui::Separator();

  // animators
  {
    ImGui::Text("Animators");

    ImGui::Separator();

    for (const auto& a : animators) {
      std::string label = std::string("Animator ") + (a._name.empty() ? a._id.str() : a._name);
      label += "##" + a._id.str();
      if (ImGui::Selectable(label.c_str(), c->getFirstSelection() == a._id)) {
        c->selection().clear();
        c->selection().emplace_back(a._id);
        c->selectionType() = logic::AneditContext::SelectionType::Animator;
      }
    }
  }

  ImGui::Separator();

  ImGui::End();
}

void SceneListGUI::renderNodeTree(util::Uuid& node, logic::AneditContext* c)
{
  const auto* n = c->scene().getNode(node);

  std::string label = n->_name.empty() ? n->_id.str() : n->_name;
  label += "##" + n->_id.str();

  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;

  if (n->_children.empty()) {
    flags |= ImGuiTreeNodeFlags_Leaf;
  }

  // Are we selected?
  if (node == c->getFirstSelection()) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }

  bool open = ImGui::TreeNodeEx(label.c_str(), flags);
  bool clicked = ImGui::IsItemClicked();

  // Context menu
  if (ImGui::BeginPopupContextItem()) {
    if (ImGui::MenuItem("Delete")) {
      deleteNodeClicked(c, node);
    }
    ImGui::EndPopup();
  }

  if (clicked) {
    c->selection().clear();
    c->selection().emplace_back(node);
    c->selectionType() = logic::AneditContext::SelectionType::Node;
  }

  if (open) {
    for (auto childId : n->_children) {
      renderNodeTree(childId, c);
    }

    ImGui::TreePop();
  }
}

void SceneListGUI::addFromPrefab(logic::AneditContext* c)
{
  /*if (ImGui::BeginPopupModal("Add from prefab")) {
    
    // List all prefabs as selectable
    auto& prefabs = c->scene().getPrefabs();
    for (const auto& p : prefabs) {
      std::string label = std::string("Prefab ") + (p._name.empty()? p._id.str() : p._name);
      label += "##" + p._id.str();
      if (ImGui::Selectable(label.c_str(), p._id == _selectedPrefab, ImGuiSelectableFlags_DontClosePopups)) {
        _selectedPrefab = p._id;
      }
    }

    if (ImGui::Button("OK")) {

      // Create a renderable from prefab
      if (_selectedPrefab) {
        auto* p = c->scene().getPrefab(_selectedPrefab);
        component::Renderable rend{};
        rend._materials = p->_materials;
        rend._model = p->_model;
        rend._skeleton = p->_skeleton;

        rend._boundingSphere = p->_boundingSphere;
        rend._visible = true;
        rend._transform = p->_transform;
        rend._tint = p->_tint;

        c->scene().addRenderable(std::move(rend));
      }

      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }*/
}

void SceneListGUI::loadSceneClicked(logic::AneditContext* c)
{
  // Open a file dialog and choose where to load from
  NFD::UniquePath outPath;

  nfdfilteritem_t filterItem[1] = { {"Anedit scene", "anescene"} };

  auto result = NFD::OpenDialog(outPath, filterItem, 1);
  if (result == NFD_OKAY) {
    c->setScenePath(outPath.get());
    c->loadSceneFrom(outPath.get());

    printf("Loading scene from %s\n", outPath.get());
  }
  else {
    printf("Could not load scene!\n");
  }
}

void SceneListGUI::addAnimatorClicked(logic::AneditContext* c)
{
  // Just add a default animator with no skele and anim
  render::asset::Animator animator{};
  animator._name = "New animator";
  animator._state = render::asset::Animator::State::Stopped;
  c->scene().addAnimator(std::move(animator));
}

void SceneListGUI::addLightClicked(logic::AneditContext* c)
{
  // Add a default light
  render::scene::Node node{};
  node._name = "Light";
  auto id = c->scene().addNode(std::move(node));

  c->scene().registry().addComponent<component::Light>(id);
  c->scene().registry().addComponent<component::Transform>(id, glm::mat4(1.0f), glm::mat4(1.0f));

  c->scene().registry().patchComponent<component::Light>(id);
  c->scene().registry().patchComponent<component::Transform>(id);
}

void SceneListGUI::deleteNodeClicked(logic::AneditContext* c, util::Uuid& node)
{
  // Scene will make sure that children are also deleted
  c->scene().removeNode(node);

  // Make sure we clear selection if this node was selected
  c->selection().clear();
  c->selectionType() = logic::AneditContext::SelectionType::Node;
}

void SceneListGUI::saveSceneClicked(logic::AneditContext* c)
{
  c->serializeScene();
}

void SceneListGUI::saveSceneAsClicked(logic::AneditContext* c)
{
  // Open a file dialog and choose where to serialize the scene
  NFD::UniquePath outPath;

  nfdfilteritem_t filterItem[1] = { {"Anedit scene", "anescene"} };

  auto result = NFD::SaveDialog(outPath, filterItem, 1, nullptr, "scene.anescene");
  if (result == NFD_OKAY) {
    c->setScenePath(outPath.get());
    c->serializeScene();

    printf("Wrote scene to %s\n", outPath.get());
  }
  else {
    printf("Could not write scene!\n");
  }
}

}