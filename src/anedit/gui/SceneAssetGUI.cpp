#include "SceneAssetGUI.h"

#include "GUIUtil.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

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
  bool& dragged)
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

    std::string label2 = std::string("Mat ") + (i._name.empty() ? i._id.str() : i._name);
    label2 += "##" + i._id.str();
    if (ImGui::Selectable(label2.c_str(), selectedUuid == i._id)) {
      selectedUuid = i._id;
      changed = true;
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
  if (drawAssetList(size, _matFilter, "Materials", c->scene().getMaterials(), _selectedMaterial, dummyUuid, dummy)) {
    c->selectedMaterial() = _selectedMaterial;
  }

  ImGui::SameLine();

  // Models
  if (drawAssetList(size, _modelFilter, "Models", c->scene().getModels(), _selectedModel, dummyUuid, dummy)) {
    // select model in context
  }

  ImGui::SameLine();

  // Animations
  if (drawAssetList(size, _animationFilter, "Animations", c->scene().getAnimations(), _selectedAnimation, dummyUuid, dummy)) {

  }

  ImGui::SameLine();

  // Skeletons
  if (drawAssetList(size, _skeletonFilter, "Skeletons", c->scene().getSkeletons(), _selectedSkeleton, dummyUuid, dummy)) {

  }

  ImGui::SameLine();

  // Prefabs
  {
    bool draggedThisFrame = false;
    if (drawAssetList(size, _prefabFilter, "Prefabs", c->scene().getPrefabs(), _selectedPrefab, _draggedPrefab, draggedThisFrame)) {
      c->selectedPrefab() = _selectedPrefab;
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