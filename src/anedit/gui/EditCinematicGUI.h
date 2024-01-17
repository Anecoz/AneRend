#pragma once

#include "IGUI.h"

#include <util/Uuid.h>

namespace gui {

class EditCinematicGUI : public IGUI
{
public:
  EditCinematicGUI();
  ~EditCinematicGUI();

  void immediateDraw(logic::AneditContext* c) override final;
private:
  void addKeyframePressed(util::Uuid cinematicId, logic::AneditContext* c);
  void playPressed(util::Uuid cinematicId, logic::AneditContext* c);

  float _time = 0.0f;
  bool _addCameraCheckbox = true;
  bool _addNodesCheckbox = true;
};

}