#include "EditCinematicGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <imgui.h>
#include <imgui_neo_sequencer.h>

namespace gui {

EditCinematicGUI::EditCinematicGUI()
  : IGUI()
{}

EditCinematicGUI::~EditCinematicGUI()
{}

void EditCinematicGUI::immediateDraw(logic::AneditContext* c)
{
  ImGui::Begin("Edit cinematic");
  auto id = c->getLastCinematicSelection();

  if (!id) {
    return;
  }

  if (!c->scene().getCinematic(id)) {
    return;
  }

  // Lazy create
  c->createCinematicPlayer(id);

  // Name field for the cinematic
  char name[100];
  name[0] = '\0';
  auto cinematic = *c->scene().getCinematic(id);
  strcpy_s(name, cinematic._name.c_str());

  _selectedKFType = None;

  ImGui::Text("Name");
  if (ImGui::InputText("##cinematic_name", name, 100) && id) {
    cinematic._name = name;
    c->scene().updateCinematic(cinematic);
  }
  ImGui::Separator();

  _time = (float)c->getCinematicTime(id);
  int32_t currentFrame = translateTimeToKey(_time);
  auto savedCurrentFrame = currentFrame;
  int32_t startFrame = 0;
  int32_t endFrame = translateTimeToKey(cinematic._maxTime);

  bool changed = false;

  int camKFToDelete = -1;
  bool addCameraKeyframeThisFrame = false;

  int nodeKfToDelete = -1;
  util::Uuid nodeToAdd;
  util::Uuid nodeToDelete;

  int matKfToDelete = -1;
  util::Uuid matToAdd;
  util::Uuid matToDelete;

  bool addKeyFrameToSelectedTimeline = false;

  if (!verifyNodesExist(cinematic, c)) {
    // Update immediately
    c->scene().updateCinematic(cinematic);
  }

  // Fill in keys
  if (!_cachedId || _cachedId != id) {
    fillCamKeys(cinematic);

    // Node key frames
    fillNodeKeys(cinematic, c);

    // Material key frames
    fillMatKeys(cinematic, c);

    _cachedId = id;
  }


  // Wrap in begin/end to create a separate window
  ImGui::Begin("Cinematic sequencer");
  if (ImGui::Button("Play")) {
    playPressed(id, c);
  }

  ImGui::SameLine();
  if (ImGui::Button("Pause")) {
    pausePressed(id, c);
  }

  ImGui::SameLine();
  if (ImGui::Button("Stop")) {
    stopPressed(id, c);
  }

  ImGui::SameLine();
  ImGui::Text("Time: %fs", _time);

  ImGui::SameLine();
  if (ImGui::Button("+")) {
    addKeyFrameToSelectedTimeline = true;
  }

  // The sequencer
  {
    if (ImGui::BeginNeoSequencer("Sequencer", &currentFrame, &startFrame, &endFrame, { 0, 0 },
      ImGuiNeoSequencerFlags_EnableSelection |
      ImGuiNeoSequencerFlags_Selection_EnableDragging |
      ImGuiNeoSequencerFlags_Selection_EnableDeletion)) {

      if (_forceSelectionClear) {
        ImGui::NeoClearSelection();
        _forceSelectionClear = false;
      }

      // Camera
      if (ImGui::BeginNeoTimelineEx("Camera")) {

        if (addKeyFrameToSelectedTimeline && ImGui::IsNeoTimelineSelected()) {
          addCameraKeyframeThisFrame = true;
        }

        for (std::size_t i = 0; i < _cachedCamKeys.size(); ++i) {
          auto& v = _cachedCamKeys[i];
          ImGui::NeoKeyframe(&v);
          // Per keyframe code here
          if (ImGui::IsNeoKeyframeSelected()) {
            _selectedKFType = Camera;
            _selectedIndex = i;

            if (ImGui::NeoIsDraggingSelection()) {
              // Update this cam key
              cinematic._camKeyframes[i]._time = translateKeyToTime(v);
              changed = true;
            }
          }

          std::string popupStr = "keyframe_context_menu_cam" + std::to_string(i);
          if (ImGui::IsNeoKeyframeRightClicked()) {
            ImGui::OpenPopup(popupStr.c_str(), 1);
          }

          if (ImGui::BeginPopup(popupStr.c_str(), ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings)) {

            if (ImGui::MenuItem("Delete")) {
              camKFToDelete = (int)i;
            }

            ImGui::EndPopup();
          }
        }

        ImGui::EndNeoTimeLine();
      }

      // Nodes
      for (auto& [id, vec] : _cachedNodeKeys) {
        std::string label = _nodeNames[id];
        if (ImGui::BeginNeoTimelineEx(label.c_str())) {

          if (addKeyFrameToSelectedTimeline && ImGui::IsNeoTimelineSelected()) {
            nodeToAdd = id;
          }

          for (std::size_t i = 0; i < vec.size(); ++i) {
            auto& v = vec[i];
            ImGui::NeoKeyframe(&v);
            // Per keyframe code here
            if (ImGui::IsNeoKeyframeSelected()) {
              _selectedKFType = Node;
              _selectedIndex = i;
              _selectedNode = id;

              if (ImGui::NeoIsDraggingSelection()) {
                cinematic._nodeKeyframes[_nodeIndices[id]][i]._time = translateKeyToTime(v);
                changed = true;
              }
            }

            std::string popupStr = "keyframe_context_menu_nodes" + std::to_string(i) + id.str();
            if (ImGui::IsNeoKeyframeRightClicked()) {
              ImGui::OpenPopup(popupStr.c_str(), 1);
            }

            if (ImGui::BeginPopup(popupStr.c_str(), ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings)) {

              if (ImGui::MenuItem("Delete")) {
                nodeKfToDelete = (int)i;
                nodeToDelete = id;
              }

              ImGui::EndPopup();
            }
          }
          ImGui::EndNeoTimeLine();
        }
      }      

      // Materials
      for (auto& [id, vec] : _cachedMatKeys) {
        std::string label = _matNames[id];
        if (ImGui::BeginNeoTimelineEx(label.c_str())) {

          if (addKeyFrameToSelectedTimeline && ImGui::IsNeoTimelineSelected()) {
            matToAdd = id;
          }

          for (std::size_t i = 0; i < vec.size(); ++i) {
            auto& v = vec[i];
            ImGui::NeoKeyframe(&v);
            // Per keyframe code here
            if (ImGui::IsNeoKeyframeSelected()) {
              _selectedKFType = Material;
              _selectedIndex = i;
              _selectedMat = id;

              if (ImGui::NeoIsDraggingSelection()) {
                cinematic._materialKeyframes[_matIndices[id]][i]._time = translateKeyToTime(v);
                changed = true;
              }
            }

            std::string popupStr = "keyframe_context_menu_mats" + std::to_string(i) + id.str();
            if (ImGui::IsNeoKeyframeRightClicked()) {
              ImGui::OpenPopup(popupStr.c_str(), 1);
            }

            if (ImGui::BeginPopup(popupStr.c_str(), ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings)) {

              if (ImGui::MenuItem("Delete")) {
                matKfToDelete = (int)i;
                matToDelete = id;
              }

              ImGui::EndPopup();
            }
          }
          ImGui::EndNeoTimeLine();
        }
      }

      ImGui::EndNeoSequencer();
    }
  } // sequencer end

  ImGui::End();

  // Draw the list of current nodes involved
  {
    if (ImGui::BeginChild("Node list", ImVec2(0, 200), true)) {
      ImGui::Text("Node list");
      ImGui::Separator();
      for (auto& pair : _nodeNames) {
        std::string label = pair.second;

        ImGui::Text(label.c_str());
      }
    }

    ImGui::EndChild();

    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("node_id")) {
        std::array<std::uint8_t, 16> arr{};
        std::memcpy(arr.data(), payload->Data, sizeof(util::Uuid));

        if (nodeDroppedInList(util::Uuid(arr), cinematic, c)) {
          changed = true;
        }
      }

      ImGui::EndDragDropTarget();
    }
  }

  // Draw the list of current materials involved
  {
    if (ImGui::BeginChild("Material list", ImVec2(0, 200), true)) {
      ImGui::Text("Material list");
      ImGui::Separator();
      for (auto& pair : _matNames) {
        std::string label = pair.second;

        ImGui::Text(label.c_str());
      }
    }

    ImGui::EndChild();

    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("material_id")) {
        std::array<std::uint8_t, 16> arr{};
        std::memcpy(arr.data(), payload->Data, sizeof(util::Uuid));

        if (materialDroppedInList(util::Uuid(arr), cinematic, c)) {
          changed = true;
        }
      }

      ImGui::EndDragDropTarget();
    }
  }

  // Draw specific GUI for the selected node
  if (_selectedKFType == Camera) {
    if (drawCameraKeyframeEditor(cinematic._camKeyframes[_selectedIndex], c)) {
      changed = true;
      _cachedCamKeys[_selectedIndex] = translateTimeToKey(cinematic._camKeyframes[_selectedIndex]._time);
    }
  }
  else if (_selectedKFType == Node) {
    auto idx = _nodeIndices[_selectedNode];
    if (drawNodeKeyframeEditor(cinematic._nodeKeyframes[idx][_selectedIndex], c)) {
      changed = true;

      _cachedNodeKeys[_selectedNode][_selectedIndex] = translateTimeToKey(cinematic._nodeKeyframes[idx][_selectedIndex]._time);
    }
  }
  else if (_selectedKFType == Material) {
    auto idx = _matIndices[_selectedMat];
    if (drawMatKeyframeEditor(cinematic._materialKeyframes[idx][_selectedIndex], c)) {
      changed = true;

      _cachedMatKeys[_selectedMat][_selectedIndex] = translateTimeToKey(cinematic._materialKeyframes[idx][_selectedIndex]._time);
    }
  }

  ImGui::End();

  // Any keyframes that should be deleted?
  if (camKFToDelete != -1) {
    cinematic._camKeyframes.erase(cinematic._camKeyframes.begin() + camKFToDelete);
    // redo cache
    fillCamKeys(cinematic);
    changed = true;
  }

  if (nodeToDelete) {
    auto idx = _nodeIndices[nodeToDelete];
    cinematic._nodeKeyframes[idx].erase(cinematic._nodeKeyframes[idx].begin() + nodeKfToDelete);
    fillNodeKeys(cinematic, c);
    changed = true;
  }

  if (matToDelete) {
    auto idx = _nodeIndices[matToDelete];
    cinematic._materialKeyframes[idx].erase(cinematic._materialKeyframes[idx].begin() + matKfToDelete);
    fillMatKeys(cinematic, c);
    changed = true;
  }

  // Add new keyframes
  if (addCameraKeyframeThisFrame) {
    addCameraKeyframePressed(cinematic, c);
  }

  if (nodeToAdd) {
    addNodeKeyframePressed(cinematic, nodeToAdd, c);
  }

  if (matToAdd) {
    addMaterialKeyframePressed(cinematic, matToAdd, c);
  }

  // Update
  if (currentFrame != savedCurrentFrame) {
    c->setCinematicTime(id, translateKeyToTime(currentFrame));
  }

  if (changed) {
    recalculateMaxTime(cinematic);
    c->setCinematicTime(id, translateKeyToTime(currentFrame));
    c->scene().updateCinematic(std::move(cinematic));
  }
}

bool EditCinematicGUI::verifyNodesExist(render::asset::Cinematic& cinematic, logic::AneditContext* c)
{
  bool ret = true;
  // Go through all node ids and check that they actually exist in the scene
  for (auto it = cinematic._nodeKeyframes.begin(); it != cinematic._nodeKeyframes.end();) {
    // Peek id
    auto& id = (*it)[0]._id;
    if (!c->scene().getNode(id)) {
      ret = false;

      // Remove from cinematic. This is quite dangerous if we get it wrong...
      printf("WARNING! Detected a cinematic node that doesn't exist, removing it from cinematic...\n");
      it = cinematic._nodeKeyframes.erase(it);
    }
    else {
      ++it;
    }
  }

  return ret;
}

void EditCinematicGUI::fillCamKeys(render::asset::Cinematic& cinematic)
{
  _cachedCamKeys.clear();
  for (auto& kf : cinematic._camKeyframes) {
    _cachedCamKeys.emplace_back(translateTimeToKey(kf._time));
  }

  _forceSelectionClear = true;
}

void EditCinematicGUI::fillMatKeys(render::asset::Cinematic& cinematic, logic::AneditContext* c)
{
  _cachedMatKeys.clear();
  _matNames.clear();
  _matIndices.clear();

  for (std::size_t i = 0; i < cinematic._materialKeyframes.size(); ++i) {
    auto& v = cinematic._materialKeyframes[i];
    // Just peek into first element of vector to get uuid
    if (!v.empty()) {
      auto uuid = v[0]._id;
      _matNames[uuid] = c->scene().getMaterial(uuid)->_name;
      _matIndices[uuid] = i;

      for (auto& kf : v) {
        _cachedMatKeys[uuid].emplace_back(translateTimeToKey(kf._time));
      }
    }
  }

  _forceSelectionClear = true;
}

void EditCinematicGUI::fillNodeKeys(render::asset::Cinematic& cinematic, logic::AneditContext* c)
{
  _cachedNodeKeys.clear();
  _nodeNames.clear();
  _nodeIndices.clear();

  for (std::size_t i = 0; i < cinematic._nodeKeyframes.size(); ++i) {
    auto& v = cinematic._nodeKeyframes[i];
    // Just peek into first element of vector to get uuid
    if (!v.empty()) {
      auto uuid = v[0]._id;
      _nodeNames[uuid] = c->scene().getNode(uuid)->_name;
      _nodeIndices[uuid] = i;

      for (auto& kf : v) {
        _cachedNodeKeys[uuid].emplace_back(translateTimeToKey(kf._time));
      }
    }
  }

  _forceSelectionClear = true;
}

std::int32_t EditCinematicGUI::translateTimeToKey(const float& time)
{
  // 0.2 seconds per key
  constexpr auto factor = 0.2f;
  return (std::int32_t)(time / factor);
}

float EditCinematicGUI::translateKeyToTime(const std::int32_t& key)
{
  constexpr auto factor = 0.2f;
  return key * factor;
}

void EditCinematicGUI::addCameraKeyframePressed(render::asset::Cinematic& cinematic, logic::AneditContext* c)
{
  printf("Adding cam keyframe at time %f\n", _time);

  render::asset::CameraKeyframe kf{};
  kf._time = _time;
  auto ypr = c->camera().getYPR();

  kf._pos = c->camera().getPosition();
  kf._ypr = ypr;
  kf._orientation = glm::quat(glm::vec3(ypr.y, ypr.x, ypr.z));

  if (_time > cinematic._maxTime) {
    cinematic._maxTime = _time;
  }

  cinematic._camKeyframes.emplace_back(std::move(kf));
    
  // Force sort
  std::sort(cinematic._camKeyframes.begin(), cinematic._camKeyframes.end(),
    [](render::asset::CameraKeyframe& a, render::asset::CameraKeyframe& b) {
      return a._time < b._time;
    });

  // Redo cam keys
  fillCamKeys(cinematic);

  c->scene().updateCinematic(cinematic);
}

void EditCinematicGUI::addNodeKeyframePressed(render::asset::Cinematic& cinematic, util::Uuid node, logic::AneditContext* c)
{
  auto* nodeP = c->scene().getNode(node);

  if (!nodeP) {
    return;
  }

  printf("Adding node keyframe at time %f\n", _time);

  render::asset::NodeKeyframe kf{};
  kf._time = _time;
  kf._id = node;
  kf._comps = c->scene().nodeToPotComps(node);

  auto idx = _nodeIndices[node];
  cinematic._nodeKeyframes[idx].emplace_back(std::move(kf));

  // Force sort
  std::sort(cinematic._nodeKeyframes[idx].begin(), cinematic._nodeKeyframes[idx].end(),
    [](render::asset::NodeKeyframe& a, render::asset::NodeKeyframe& b) {
      return a._time < b._time;
    });

  // Redo cache
  fillNodeKeys(cinematic, c);

  c->scene().updateCinematic(cinematic);
}

void EditCinematicGUI::addMaterialKeyframePressed(render::asset::Cinematic& cinematic, util::Uuid material, logic::AneditContext* c)
{
  auto* matP = c->scene().getMaterial(material);

  if (!matP) {
    return;
  }

  printf("Adding material keyframe at time %f\n", _time);

  render::asset::MaterialKeyframe kf{};
  kf._time = _time;
  kf._id = material;
  kf._emissive = matP->_emissive;
  kf._baseColFactor = matP->_baseColFactor;

  auto idx = _matIndices[material];
  cinematic._materialKeyframes[idx].emplace_back(std::move(kf));

  // Force sort
  std::sort(cinematic._materialKeyframes[idx].begin(), cinematic._materialKeyframes[idx].end(),
    [](render::asset::MaterialKeyframe& a, render::asset::MaterialKeyframe& b) {
      return a._time < b._time;
    });

  // Redo cache
  fillMatKeys(cinematic, c);

  c->scene().updateCinematic(cinematic);
}

bool EditCinematicGUI::nodeDroppedInList(util::Uuid node, render::asset::Cinematic& cinematic, logic::AneditContext* c)
{
  // Check that we don't have this node already
  if (_nodeIndices.find(node) != _nodeIndices.end()) {
    return false;
  }

  // Add this node and a kf at time 0
  const auto* nodeP = c->scene().getNode(node);
  if (!nodeP) {
    return false;
  }

  render::asset::NodeKeyframe kf{};
  kf._time = 0.0f;
  kf._id = node;
  kf._comps = c->scene().nodeToPotComps(node);

  // Also add to cache
  _nodeIndices[node] = cinematic._nodeKeyframes.size();
  _nodeNames[node] = nodeP->_name;
  _cachedNodeKeys[node].emplace_back(0);

  // Assume this node not already here
  cinematic._nodeKeyframes.emplace_back();
  cinematic._nodeKeyframes.back().emplace_back(std::move(kf));

  return true;
}

bool EditCinematicGUI::materialDroppedInList(util::Uuid material, render::asset::Cinematic& cinematic, logic::AneditContext* c)
{
  // Check that we don't have this material already
  if (_matIndices.find(material) != _matIndices.end()) {
    return false;
  }

  // Add this material and a kf at time 0
  const auto* matP = c->scene().getMaterial(material);
  if (!matP) {
    return false;
  }

  render::asset::MaterialKeyframe kf{};
  kf._time = 0.0f;
  kf._id = material;
  kf._emissive = matP->_emissive;
  kf._baseColFactor = matP->_baseColFactor;

  // Also add to cache
  _matIndices[material] = cinematic._materialKeyframes.size();
  _matNames[material] = matP->_name;
  _cachedMatKeys[material].emplace_back(0);

  // Assume this material not already here
  cinematic._materialKeyframes.emplace_back();
  cinematic._materialKeyframes.back().emplace_back(std::move(kf));

  return true;
}

void EditCinematicGUI::playPressed(util::Uuid cinematicId, logic::AneditContext* c)
{
  c->playCinematic(cinematicId);
}

void EditCinematicGUI::pausePressed(util::Uuid cinematicId, logic::AneditContext* c)
{
  c->pauseCinematic(cinematicId);
}

void EditCinematicGUI::stopPressed(util::Uuid cinematicId, logic::AneditContext* c)
{
  c->stopCinematic(cinematicId);
}

bool EditCinematicGUI::drawEasingCombo(util::interp::Easing& easing)
{
  bool easingChanged = false;
  const std::vector<util::interp::Easing> easings{
      util::interp::Easing::Linear,
      util::interp::Easing::InOutSine,
      util::interp::Easing::InCubic,
      util::interp::Easing::OutCubic,
      util::interp::Easing::InOutCubic,
      util::interp::Easing::InOutElastic,
      util::interp::Easing::OutBounce,
      util::interp::Easing::InOutBounce
  };

  auto str = util::interp::easingToStr(easing);

  if (ImGui::BeginCombo("Easing", str.c_str())) {
    for (int i = 0; i < easings.size(); ++i) {
      auto easingStr = util::interp::easingToStr(easings[i]);
      bool isSelected = str == easingStr;

      if (ImGui::Selectable(easingStr.c_str(), isSelected)) {
        easing = easings[i];
        easingChanged = true;
      }

      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }

    ImGui::EndCombo();
  }

  return easingChanged;
}

bool EditCinematicGUI::drawCameraKeyframeEditor(render::asset::CameraKeyframe& kf, logic::AneditContext* c)
{
  bool changed = false;
  ImGui::BeginChild("camera_kf", ImVec2(0, 0), true);
  ImGui::Text("Edit camera key");
  ImGui::Separator();

  if (drawEasingCombo(kf._easing)) {
    changed = true;
  }

  ImGui::Separator();

  if (ImGui::InputFloat("Time", &kf._time)) {
    changed = true;
  }

  // Just sets new parameters
  if (ImGui::Button("Patch")) {
    auto ypr = c->camera().getYPR();

    kf._pos = c->camera().getPosition();
    kf._ypr = ypr;
    kf._orientation = glm::quat(glm::vec3(ypr.y, ypr.x, ypr.z));

    changed = true;
  }

  ImGui::EndChild();

  return changed;
}

bool EditCinematicGUI::drawNodeKeyframeEditor(render::asset::NodeKeyframe& kf, logic::AneditContext* c)
{
  bool changed = false;

  ImGui::BeginChild("node_kf", ImVec2(0, 0), true);
  ImGui::Text("Edit node key");
  ImGui::Separator();

  if (drawEasingCombo(kf._easing)) {
    changed = true;
  }

  ImGui::Separator();

  if (ImGui::InputFloat("Time", &kf._time)) {
    changed = true;
  }

  // Just sets new parameters
  if (ImGui::Button("Patch")) {
    kf._comps = c->scene().nodeToPotComps(kf._id);

    changed = true;
  }

  ImGui::EndChild();

  return changed;
}

bool EditCinematicGUI::drawMatKeyframeEditor(render::asset::MaterialKeyframe& kf, logic::AneditContext* c)
{
  bool changed = false;

  ImGui::BeginChild("mat_kf", ImVec2(0, 0), true);
  ImGui::Text("Edit material key");
  ImGui::Separator();

  if (drawEasingCombo(kf._easing)) {
    changed = true;
  }

  ImGui::Separator();

  if (ImGui::InputFloat("Time", &kf._time)) {
    changed = true;
  }

  // Just sets new parameters
  if (ImGui::Button("Patch")) {
    auto* matP = c->scene().getMaterial(kf._id);
    kf._emissive = matP->_emissive;
    kf._baseColFactor = matP->_baseColFactor;

    changed = true;
  }

  ImGui::EndChild();

  return changed;
}

void EditCinematicGUI::recalculateMaxTime(render::asset::Cinematic& cinematic)
{
  float max = 0.0f;

  for (auto& kf : cinematic._camKeyframes) {
    if (kf._time > max) {
      max = kf._time;
    }
  }

  for (auto& v : cinematic._materialKeyframes) {
    for (auto& kf : v) {
      if (kf._time > max) {
        max = kf._time;
      }
    }
  }

  for (auto& v : cinematic._nodeKeyframes) {
    for (auto& kf : v) {
      if (kf._time > max) {
        max = kf._time;
      }
    }
  }

  cinematic._maxTime = max;
}

}