#include "SceneAssetGUI.h"

#include "GUIUtil.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <functional>

#include <imgui.h>
#include <nfd.hpp>

namespace gui {

namespace {

// Returns if selectedUuid was changed
template <typename T>
bool drawAssetList(
  const ImVec2& size, 
  std::string& filter,
  const char* label, 
  const std::vector<T>& v, 
  util::Uuid& selectedUuid,
  util::Uuid& draggedUuid, // not necessarily the same as selected
  bool& dragged,
  std::vector<std::string> contextMenuItems = {},
  std::vector<std::function<void(util::Uuid)>> contextMenuCbs = {})
{
  bool changed = false;
  ImGui::BeginChild(label, size, true);
  ImGui::Text(label);

  char buf[40];
  strcpy_s(buf, filter.c_str());
  if (ImGui::InputText("Filter", buf, 40)) {
    filter = std::string(buf);

    std::transform(filter.begin(), filter.end(), filter.begin(),
      [](unsigned char c) { return std::tolower(c); });
  }

  ImGui::Separator();

  for (const auto& i : v) {

    if (!filter.empty()) {
      std::string name = i._name;

      std::transform(name.begin(), name.end(), name.begin(),
        [](unsigned char c) { return std::tolower(c); });

      if (name.find(filter) == std::string::npos) {
        continue;
      }
    }

    std::string label2 = (i._name.empty() ? i._id.str() : i._name);
    label2 += "##" + i._id.str();
    if (ImGui::Selectable(label2.c_str(), selectedUuid == i._id)) {
      selectedUuid = i._id;
      changed = true;
    }

    // Context menu
    if (!contextMenuCbs.empty()) {
      if (ImGui::BeginPopupContextItem()) {
        for (std::size_t j = 0; j < contextMenuItems.size(); ++j) {
          if (ImGui::MenuItem(contextMenuItems[j].c_str())) {
            contextMenuCbs[j](i._id);
          }
        }

        ImGui::EndPopup();
      }
    }

    // drag&drop
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
      ImGui::Text(label2.c_str());

      dragged = true;
      draggedUuid = i._id;

      ImGui::EndDragDropSource();
    }
  }

  ImGui::EndChild();

  return changed;
}

}

SceneAssetGUI::SceneAssetGUI()
  : IGUI()
{}

SceneAssetGUI::~SceneAssetGUI()
{}

void SceneAssetGUI::immediateDraw(logic::AneditContext* c)
{
  const int numPanels = 5;

  ImGui::Begin("Assets", NULL, ImGuiWindowFlags_MenuBar);
  ImVec2 size = ImVec2(ImGui::GetWindowWidth() / numPanels, 0);

  // Menu
  if (ImGui::BeginMenuBar()) {

    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load GLTF...")) {
        loadGLTFClicked(c);
      }

      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }

  // Materials
  bool dummy;
  util::Uuid dummyUuid;
  auto currSelection = c->getFirstSelection();
  if (drawAssetList(size, _matFilter, "Materials", c->scene().getMaterials(), currSelection, dummyUuid, dummy)) {
    c->selection().clear();
    c->selection().emplace_back(currSelection);
    c->selectionType() = logic::AneditContext::SelectionType::Material;
  }

  ImGui::SameLine();

  // Models
  if (drawAssetList(size, _modelFilter, "Models", c->scene().getModels(), currSelection, dummyUuid, dummy)) {
    // select model in context
  }

  ImGui::SameLine();

  // Animations
  if (drawAssetList(size, _animationFilter, "Animations", c->scene().getAnimations(), currSelection, dummyUuid, dummy)) {

  }

  ImGui::SameLine();

  // Skeletons
  if (drawAssetList(size, _skeletonFilter, "Skeletons", c->scene().getSkeletons(), currSelection, dummyUuid, dummy)) {

  }

  ImGui::SameLine();

  // Prefabs
  {
    auto deleteLambda = [c](util::Uuid id) { c->scene().removePrefab(id); c->selection().clear(); };

    std::vector<std::string> menuItems = { "Delete" };
    std::vector<std::function<void(util::Uuid)>> cbs = { deleteLambda };

    bool draggedThisFrame = false;
    if (drawAssetList(size, _prefabFilter, "Prefabs", c->scene().getPrefabs(), currSelection, _draggedPrefab, draggedThisFrame, menuItems, cbs)) {
      c->selection().clear();
      c->selection().emplace_back(currSelection);
      c->selectionType() = logic::AneditContext::SelectionType::Prefab;
    }

    // Did drag status change?
    if (_draggingPrefab && !draggedThisFrame && _draggedPrefab) {
      c->spawnFromPrefabAtMouse(_draggedPrefab);

      // Reset dragged prefab id
      _draggedPrefab = util::Uuid();
    }

    _draggingPrefab = draggedThisFrame;
  }

  ImGui::End();
}

void SceneAssetGUI::loadGLTFClicked(logic::AneditContext* c)
{
  // Open a file dialog and ask context to load GLTF
  NFD::UniquePath outPath;
  nfdfilteritem_t filterItem[1] = { {"GLTF", "gltf,glb"} };

  auto result = NFD::OpenDialog(outPath, filterItem, 1);
  if (result == NFD_OKAY) {
    c->startLoadGLTF(outPath.get());
  }
}

}