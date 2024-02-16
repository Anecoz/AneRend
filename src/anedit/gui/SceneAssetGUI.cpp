#include "SceneAssetGUI.h"

#include "GUIUtil.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>
#include <render/ImageHelpers.h>
#include <util/TextureHelpers.h>

#include <filesystem>
#include <functional>
#include <unordered_map>

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
  const char* dropSourceType = "",
  std::vector<std::string> contextMenuItems = {},
  std::vector<std::function<void(util::Uuid)>> contextMenuCbs = {},
  const char* dropTargetType = "",
  std::vector<std::uint8_t>* dropTargetData = nullptr)
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

      if (strlen(dropSourceType) != 0) {
        ImGui::SetDragDropPayload(dropSourceType, &i._id, sizeof(util::Uuid));
      }

      dragged = true;
      draggedUuid = i._id;

      ImGui::EndDragDropSource();
    }

    if (dropTargetData) {
      if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dropTargetType)) {
          dropTargetData->resize(payload->DataSize);
          std::memcpy(dropTargetData->data(), payload->Data, payload->DataSize);
        }

        ImGui::EndDragDropTarget();
      }
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
      if (ImGui::MenuItem("New material")) {
        createMaterialClicked(c);
      }

      if (ImGui::MenuItem("Load GLTF...")) {
        loadGLTFClicked(c);
      }

      if (ImGui::MenuItem("Load RGBA texture...")) {
        loadRGBATextureClicked(c);
      }

      if (ImGui::MenuItem("Load grayscale texture...")) {
        loadGrayTextureClicked(c);
      }

      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }

  // Materials
  bool dummy;
  util::Uuid dummyUuid;
  auto currSelection = c->getFirstSelection();
  if (drawAssetList(size, _matFilter, "Materials", c->scene().getMaterials(), currSelection, dummyUuid, dummy, "material_id")) {
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

  // Textures
  if (drawAssetList(size, _textureFilter, "Textures", c->scene().getTextures(), currSelection, dummyUuid, dummy, "texture_id")) {
    c->selection().clear();
    c->selection().emplace_back(currSelection);
    c->selectionType() = logic::AneditContext::SelectionType::Texture;
  }

  ImGui::SameLine();

  // Prefabs
  {
    auto deleteLambda = [c](util::Uuid id) { c->scene().removePrefab(id); c->selection().clear(); };

    std::vector<std::string> menuItems = { "Delete" };
    std::vector<std::function<void(util::Uuid)>> cbs = { deleteLambda };
    std::vector<std::uint8_t> dropData;

    bool draggedThisFrame = false;
    if (drawAssetList(
      size, 
      _prefabFilter, 
      "Prefabs", 
      c->scene().getPrefabs(), 
      currSelection, 
      _draggedPrefab,
      draggedThisFrame,
      "",
      menuItems, 
      cbs,
      "node_id",
      &dropData)) {
      c->selection().clear();
      c->selection().emplace_back(currSelection);
      c->selectionType() = logic::AneditContext::SelectionType::Prefab;
    }

    if (!dropData.empty()) {
      // Got a node_id dropped on us
      std::array<std::uint8_t, 16> arr{};
      std::memcpy(arr.data(), dropData.data(), sizeof(util::Uuid));

      nodeDroppedOnPrefab(c, util::Uuid(arr));
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

namespace {

std::string pathToName(std::string name)
{
  return std::filesystem::path(name).stem().string();
}

void addTextureToScene(logic::AneditContext* c, render::imageutil::TextureData&& data, std::string&& name)
{
  if (!data) {
    return;
  }

  auto format = render::asset::Texture::Format::RGBA8_UNORM;
  if (!data.isColor) {
    if (data.bitdepth == 16) {
      format = render::asset::Texture::Format::R16_UNORM;
    }
    else {
      format = render::asset::Texture::Format::R8_UNORM;
    }
  }

  render::asset::Texture tex{};
  tex._data.emplace_back(std::move(data.data));
  tex._height = data.height;
  tex._width = data.width;
  tex._name = std::move(name);
  tex._numMips = 1;
  tex._format = format;

  // Generate mipmaps. TODO: Maybe not always?
  c->generateMipMaps(tex);

  // And then compress
  if (render::imageutil::numDimensions(tex._format) >= 3) {
    util::TextureHelpers::convertRGBA8ToBC7(tex);
  }
  else if (render::imageutil::numDimensions(tex._format) == 2) {
    util::TextureHelpers::convertRG8ToBC5(tex);
  }

  c->scene().addTexture(std::move(tex));
}

}

void SceneAssetGUI::loadGrayTextureClicked(logic::AneditContext* c)
{
  NFD::UniquePath outPath;
  nfdfilteritem_t filterItem[1] = { {"PNG", "png"} };

  auto result = NFD::OpenDialog(outPath, filterItem, 1);
  if (result == NFD_OKAY) {
    // Just raw dog texture loading on current thread
    std::string path = outPath.get();
    addTextureToScene(c, render::imageutil::loadTex(path, true), pathToName(path));
  }
}

void SceneAssetGUI::createMaterialClicked(logic::AneditContext* c)
{
  // Just create an empty default material
  render::asset::Material mat{};
  mat._name = "NewMaterial";
  c->scene().addMaterial(std::move(mat));
}

void SceneAssetGUI::loadRGBATextureClicked(logic::AneditContext* c)
{
  NFD::UniquePath outPath;
  nfdfilteritem_t filterItem[1] = { {"JPEG, PNG", "jpg,jpeg,png"}};

  auto result = NFD::OpenDialog(outPath, filterItem, 1);
  if (result == NFD_OKAY) {
    // Just raw dog texture loading on current thread
    std::string path = outPath.get();
    addTextureToScene(c, render::imageutil::loadTex(path), pathToName(path));
  }
}

namespace {

// Returns prefab id
util::Uuid prefabFromNodeRecurse(
  logic::AneditContext* c, 
  util::Uuid node, util::Uuid parent, 
  std::vector<render::asset::Prefab>& out,
  std::unordered_map<util::Uuid, util::Uuid>& prefabMapOut) // <node, prefab>
{
  auto nodeP = c->scene().getNode(node);

  if (!nodeP) {
    return {};
  }

  auto parentPrefab = c->prefabFromNode(node);
  parentPrefab._parent = parent;

  prefabMapOut[node] = parentPrefab._id;

  for (auto& child : nodeP->_children) {
    parentPrefab._children.emplace_back(
      prefabFromNodeRecurse(c, child, node, out, prefabMapOut)
    );    
  }

  auto idOut = parentPrefab._id;
  out.emplace_back(std::move(parentPrefab));
  return idOut;
}

}

void SceneAssetGUI::nodeDroppedOnPrefab(logic::AneditContext* c, util::Uuid node)
{
  // This means we want to create prefabs from the given node.
  auto nodeP = c->scene().getNode(node);

  if (!nodeP) {
    return;
  }

  // Create prefabs from node and all its children recursively
  std::vector<render::asset::Prefab> prefabs;
  std::unordered_map<util::Uuid, util::Uuid> nodePrefabMap;
  prefabFromNodeRecurse(c, node, {}, prefabs, nodePrefabMap);

  // If there was a skeleton component somewhere, update the joint refs to point to the prefabs instead
  for (auto& p : prefabs) {
    if (p._comps._skeleton) {

      auto& skeleComp = p._comps._skeleton.value();
      for (auto& jr : skeleComp._jointRefs) {
        assert(nodePrefabMap.find(jr._node) != nodePrefabMap.end() && "Can't update skeleton, something really wrong!");

        jr._node = nodePrefabMap[jr._node];
      }
    }
  }

  for (auto& p : prefabs) {
    c->scene().addPrefab(std::move(p));
  }
}

}