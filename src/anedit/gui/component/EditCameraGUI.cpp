#include "EditCameraGUI.h"

#include "../../logic/AneditContext.h"
#include <render/scene/Scene.h>

#include <imgui.h>

namespace gui {

EditCameraGUI::EditCameraGUI()
  : IGUI()
{}

EditCameraGUI::~EditCameraGUI()
{}

void EditCameraGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->getFirstSelection();

  if (!id) {
    return;
  }

  bool changed = false;
  auto& cameraComp = c->scene().registry().getComponent<component::Camera>(id);

  // Halfheight
  if (ImGui::InputFloat("FOV (degrees)", &cameraComp._fov)) {
    cameraComp._fov = glm::clamp(cameraComp._fov, 30.0f, 110.0f);
    changed = true;
  }

  if (changed) {
    c->scene().registry().patchComponent<component::Camera>(id);
  }
}

}