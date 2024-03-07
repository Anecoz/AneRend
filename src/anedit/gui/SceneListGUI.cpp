#include "SceneListGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>
#include <render/asset/AssetCollection.h>

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
  auto& cinematicMetas = c->assetCollection().getMetaInfos(render::asset::AssetMetaInfo::Type::Cinematic);
  const auto& nodes = c->scene().getNodes();

  ImGui::Begin("Scene list", NULL, ImGuiWindowFlags_MenuBar);

  // Menu
  if (ImGui::BeginMenuBar()) {

    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Add scene node...")) {
        addNodeClicked(c);
      }
      if (ImGui::MenuItem("Add cinematic...")) {
        addCinematicClicked(c);
      }
      if (ImGui::MenuItem("Add light...")) {
        addLightClicked(c);
      }
      if (ImGui::MenuItem("Add empty node...")) {
        addEmptyClicked(c);
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

  // cinematics
  {
    ImGui::Text("Cinematics");

    ImGui::Separator();

    for (const auto& a : cinematicMetas) {
      //auto cin = c->assetCollection().getCinematicBlocking(a);
      std::string label = std::string("Cinematic ") + (a._name.empty() ? a._id.str() : a._name);
      label += "##" + a._id.str();
      if (ImGui::Selectable(label.c_str(), c->getFirstSelection() == a._id)) {
        c->getLastCinematicSelection() = a._id;
      }
    }
  }

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
    auto n = c->scene().getNode(node);
    if (n->_parent && ImGui::MenuItem("Remove parent")) {
      auto pId = n->_parent;
      c->scene().removeNodeChild(pId, node);
    }
    ImGui::EndPopup();
  }

  if (clicked) {
    c->selection().clear();
    c->selection().emplace_back(node);
    c->selectionType() = logic::AneditContext::SelectionType::Node;
  }

  // Drag & drop
  if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
    // This should copy the data, so no worries that id won't live past this function
    ImGui::SetDragDropPayload("node_id", &node, sizeof(util::Uuid));

    ImGui::EndDragDropSource();
  }

  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("node_id")) {
      std::array<std::uint8_t, 16> arr{};
      std::memcpy(arr.data(), payload->Data, sizeof(util::Uuid));
      util::Uuid droppedId(arr);

      nodeDraggedToNode(c, droppedId, node);
    }

    ImGui::EndDragDropTarget();
  }

  if (open) {
    for (auto childId : n->_children) {
      renderNodeTree(childId, c);
    }

    ImGui::TreePop();
  }
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

void SceneListGUI::addEmptyClicked(logic::AneditContext* c)
{
  // Add a default node
  render::scene::Node node{};
  node._name = "EmptyNode";
  auto id = c->scene().addNode(std::move(node));

  c->scene().registry().addComponent<component::Transform>(id, glm::mat4(1.0f), glm::mat4(1.0f));
  c->scene().registry().patchComponent<component::Transform>(id);
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

void SceneListGUI::addNodeClicked(logic::AneditContext* c)
{  
  // Add a node with just a transform
  render::scene::Node node{};
  node._name = "EmptyNode";
  auto id = c->scene().addNode(std::move(node));
  c->scene().registry().addComponent<component::Transform>(id, glm::mat4(1.0f), glm::mat4(1.0f));
  c->scene().registry().patchComponent<component::Transform>(id);
}

void SceneListGUI::addCinematicClicked(logic::AneditContext* c)
{
  // Default cinematic
  render::asset::Cinematic cinematic{};
  cinematic._name = "New cinematic";
  //c->scene().addCinematic(std::move(cinematic));
  c->assetCollection().add(std::move(cinematic));
}

void SceneListGUI::deleteNodeClicked(logic::AneditContext* c, util::Uuid& node)
{
  // Scene will make sure that children are also deleted
  c->scene().removeNode(node);

  // Make sure we clear selection if this node was selected
  c->selection().clear();
  c->selectionType() = logic::AneditContext::SelectionType::Node;
}

void SceneListGUI::nodeDraggedToNode(logic::AneditContext* c, util::Uuid& draggedNode, util::Uuid& droppedOnNode)
{
  // draggedNode = child, droppedOnNode = parent
  c->scene().setNodeAsChild(droppedOnNode, draggedNode);
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