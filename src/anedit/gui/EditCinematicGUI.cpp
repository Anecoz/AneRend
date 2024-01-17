#include "EditCinematicGUI.h"

#include "../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <imgui.h>

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
    return;
  }

  // Name field for the cinematic
  char name[100];
  name[0] = '\0';
  const auto* node = c->scene().getCinematic(id);
  strcpy_s(name, node->_name.c_str());

  ImGui::Text("Name");
  if (ImGui::InputText("##cinematic_name", name, 100) && id) {
    auto copy = *node;
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

}