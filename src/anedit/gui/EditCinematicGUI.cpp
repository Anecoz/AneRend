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
  auto id = c->getFirstSelection();

  if (!id || (c->selectionType() != logic::AneditContext::SelectionType::Cinematic)) {
    _cachedKeys.clear();
    return;
  }

  // Lazy create
  c->createCinematicPlayer(id);

  // Name field for the cinematic
  char name[100];
  name[0] = '\0';
  const auto* cinematic = c->scene().getCinematic(id);
  strcpy_s(name, cinematic->_name.c_str());

  ImGui::Text("Name");
  if (ImGui::InputText("##cinematic_name", name, 100) && id) {
    auto copy = *cinematic;
    copy._name = name;
    c->scene().updateCinematic(std::move(copy));
  }
  ImGui::Separator();

  // Test button to simply add a key frame

  ImGui::Text("Add keyframe");
  ImGui::Checkbox("Nodes", &_addNodesCheckbox);
  ImGui::SameLine();
  ImGui::Checkbox("Camera", &_addCameraCheckbox);

  ImGui::InputFloat("Time", &_time);
  ImGui::SameLine();
  if (ImGui::Button("+")) {
    addKeyframePressed(id, c);
  }

  ImGui::Separator();
  if (ImGui::Button("Play")) {
    playPressed(id, c);
  }

  // Lab with sequencer
  // Wrap in begin/end to create a separate window
  ImGui::Begin("Cinematic editor");

  float currentTime = (float)c->getCinematicTime(id);
  int32_t currentFrame = translateTimeToKey(currentTime);
  auto savedCurrentFrame = currentFrame;
  int32_t startFrame = 0;
  int32_t endFrame = translateTimeToKey(cinematic->_keyframes.back()._time);

  // Fill in keys
  if (_cachedKeys.empty()) {
    for (auto& kf : cinematic->_keyframes) {
      _cachedKeys.emplace_back(translateTimeToKey(kf._time));
    }
  }

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
  ImGui::Text("Time: %fs", currentTime);

  if (ImGui::BeginNeoSequencer("Sequencer", &currentFrame, &startFrame, &endFrame, { 0, 0 },
    ImGuiNeoSequencerFlags_EnableSelection |
    ImGuiNeoSequencerFlags_Selection_EnableDragging |
    ImGuiNeoSequencerFlags_Selection_EnableDeletion)) {

    /*if (ImGui::BeginNeoTimeline("Keys", _cachedKeys)) {
      ImGui::EndNeoTimeLine();
    }*/
    if (ImGui::BeginNeoTimelineEx("Keys")) {
      for (auto& v : _cachedKeys) {
        ImGui::NeoKeyframe(&v);
        // Per keyframe code here
        if (ImGui::IsNeoKeyframeSelected()) {
          if (ImGui::NeoIsDraggingSelection()) {
            printf("Yeppers\n");
          }
        }
      }

      ImGui::EndNeoTimeLine();
    }

    ImGui::EndNeoSequencer();
  }

  ImGui::End();

  // Update
  if (currentFrame != savedCurrentFrame) {
    c->setCinematicTime(id, translateKeyToTime(currentFrame));
  }

  // I don't see any way to get notified if a keyframe has changed so we just redo them all...?
  auto keyFrames = cinematic->_keyframes;
  for (std::size_t i = 0; i < _cachedKeys.size(); ++i) {
    keyFrames[i]._time = translateKeyToTime(_cachedKeys[i]);
  }

  auto copy = *cinematic;
  copy._keyframes = std::move(keyFrames);
  c->scene().updateCinematic(std::move(copy));

  //c->setCinematicTime(id, translateKeyToTime(currentFrame));
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

void EditCinematicGUI::addKeyframePressed(util::Uuid cinematicId, logic::AneditContext* c)
{
  auto cinematic = *c->scene().getCinematic(cinematicId);
  render::asset::Cinematic::Keyframe topKf{};
  topKf._time = _time;

  printf("Adding keyframe at time %f\n", _time);

  if (_addCameraCheckbox) {
    auto ypr = c->camera().getYPR();

    render::asset::CameraKeyframe kf{};
    kf._pos = c->camera().getPosition();
    kf._ypr = ypr;
    kf._orientation = glm::quat(glm::vec3(ypr.y, ypr.x, ypr.z));

    topKf._camKF = std::move(kf);
  }

  cinematic._keyframes.emplace_back(std::move(topKf));

  // TODO: Everything else KEKL

  c->scene().updateCinematic(std::move(cinematic));
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

}