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
  auto id = c->getFirstSelection();
  if (!id || (c->selectionType() != logic::AneditContext::SelectionType::Node)) {
    return;
  }

  auto& animatorComp = c->scene().registry().getComponent<component::Animator>(id);

  std::vector<std::string> animations;
  std::string currentAnimationStr = c->scene().getAnimation(animatorComp._currentAnimation)->_name;
  component::Animator::State state = animatorComp._state;
  std::size_t selectedAnimIdx = 0;
  for (std::size_t i = 0; i < animatorComp._animations.size(); ++i) {
    auto& anim = animatorComp._animations[i];
    animations.emplace_back(c->scene().getAnimation(anim)->_name);
    if (anim == animatorComp._currentAnimation) {
      selectedAnimIdx = i;
    }
  }

  

  bool changed = false;
  
  if (ImGui::Button("Play")) {
    state = component::Animator::State::Playing;
    changed = true;
  }

  ImGui::SameLine();
  if (ImGui::Button("Pause")) {
    state = component::Animator::State::Paused;
    changed = true;
  }

  ImGui::SameLine();
  if (ImGui::Button("Stop")) {
    state = component::Animator::State::Stopped;
    changed = true;
  }

  if (ImGui::SliderFloat("Playback speed", &animatorComp._playbackMultiplier, 0.0f, 10.0f)) {
    changed = true;
  }

  ImGui::Separator();
  ImGui::Text("Selected animation: %s", currentAnimationStr.c_str());

  ImGui::BeginChild("Animation list", ImVec2(0, 0), true);
  for (std::size_t i = 0; i < animations.size(); ++i) {
    if (ImGui::Selectable(animations[i].c_str(), selectedAnimIdx == i)) {
      printf("Selected anim %zu (%s)\n", i, animations[i].c_str());
      selectedAnimIdx = i;
      changed = true;
    }
  }
  ImGui::EndChild();

  if (changed) {
    animatorComp._state = state;
    animatorComp._currentAnimation = animatorComp._animations[selectedAnimIdx];
    c->scene().registry().patchComponent<component::Animator>(id);
  }
}
}