#include "EditAnimatorGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <imgui.h>

namespace gui {

EditAnimatorGUI::EditAnimatorGUI()
  : IGUI()
{}

EditAnimatorGUI::~EditAnimatorGUI()
{}

void EditAnimatorGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->selectedAnimator();

  ImGui::Begin("EditAnimator");

  std::string name;
  util::Uuid skele;
  util::Uuid anim;
  render::asset::Animator::State state;
  float playbackMultiplier = 1.0f;

  bool changed = false;

  if (!id) {
    ImGui::BeginDisabled();
  }
  else {
    auto* animator = c->scene().getAnimator(id);

    name = animator->_name;
    skele = animator->_skeleId;
    anim = animator->_animId;
    state = animator->_state;
    playbackMultiplier = animator->_playbackMultiplier;
  }

  ImGui::Text("Name");
  char nameBuf[40];
  strcpy_s(nameBuf, name.c_str());
  if (ImGui::InputText("##name", nameBuf, 40)) {
    changed = true;
  }
  name = std::string(nameBuf);

  ImGui::Separator();
  if (ImGui::Button("Play")) {
    state = render::asset::Animator::State::Playing;
    changed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Pause")) {
    state = render::asset::Animator::State::Paused;
    changed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Stop")) {
    state = render::asset::Animator::State::Stopped;
    changed = true;
  }

  ImGui::Separator();
  ImGui::Text("Playback speed");
  if (ImGui::SliderFloat("##playbackspeed", &playbackMultiplier, 0.0f, 2.0f)) {
    changed = true;
  }

  ImGui::Separator();
  if (ImGui::Button("Skeleton")) {
    ImGui::OpenPopup("Choose skeleton");
  }
  if (chooseSkeleton(c, skele)) {
    changed = true;
  }

  ImGui::SameLine();
  if (ImGui::Button("Animation")) {
    ImGui::OpenPopup("Choose animation");
  }
  if (chooseAnimation(c, anim)) {
    changed = true;
  }

  if (!id) {
    ImGui::EndDisabled();
  }

  ImGui::End();

  if (changed && id) {
    auto animCopy = *c->scene().getAnimator(id);
    
    animCopy._name = name;
    animCopy._skeleId = skele;
    animCopy._animId = anim;
    animCopy._playbackMultiplier = playbackMultiplier;
    animCopy._state = state;

    c->scene().updateAnimator(std::move(animCopy));
  }
}

bool EditAnimatorGUI::chooseSkeleton(logic::AneditContext* c, util::Uuid& uuidOut)
{
  bool ret = false;

  if (ImGui::BeginPopupModal("Choose skeleton")) {

    // List all current skeletons
    const auto& skeletons = c->scene().getSkeletons();
    for (const auto& s : skeletons) {
      std::string label = std::string("Skeleton ") + (s._name.empty() ? s._id.str() : s._name);
      label += "##" + s._id.str();
      if (ImGui::Selectable(label.c_str(), s._id == _selectedSkeleton, ImGuiSelectableFlags_DontClosePopups)) {
        _selectedSkeleton = s._id;
      }
    }

    if (ImGui::Button("OK")) {
      uuidOut = _selectedSkeleton;
      ret = true;
      ImGui::CloseCurrentPopup();
    }

    if (ImGui::Button("Cancel")) {
      ret = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }

  return ret;
}

bool EditAnimatorGUI::chooseAnimation(logic::AneditContext* c, util::Uuid& uuidOut)
{
  bool ret = false;

  if (ImGui::BeginPopupModal("Choose animation")) {

    // List all current anims
    const auto& animations = c->scene().getAnimations();
    for (const auto& s : animations) {
      std::string label = std::string("Animation ") + (s._name.empty() ? s._id.str() : s._name);
      label += "##" + s._id.str();
      if (ImGui::Selectable(label.c_str(), s._id == _selectedAnimation, ImGuiSelectableFlags_DontClosePopups)) {
        _selectedAnimation = s._id;
      }
    }

    if (ImGui::Button("OK")) {
      ret = true;
      uuidOut = _selectedAnimation;
      ImGui::CloseCurrentPopup();
    }

    if (ImGui::Button("Cancel")) {
      ret = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }

  return ret;
}

}