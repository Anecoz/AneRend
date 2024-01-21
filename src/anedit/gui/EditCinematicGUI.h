#pragma once

#include "IGUI.h"

#include <util/Uuid.h>

#include <cstdint>

namespace gui {

class EditCinematicGUI : public IGUI
{
public:
  EditCinematicGUI();
  ~EditCinematicGUI();

  void immediateDraw(logic::AneditContext* c) override final;
private:
  std::int32_t translateTimeToKey(const float& time);
  float translateKeyToTime(const std::int32_t& key);
  void addKeyframePressed(util::Uuid cinematicId, logic::AneditContext* c);
  void playPressed(util::Uuid cinematicId, logic::AneditContext* c);
  void pausePressed(util::Uuid cinematicId, logic::AneditContext* c);
  void stopPressed(util::Uuid cinematicId, logic::AneditContext* c);

  float _time = 0.0f;
  bool _addCameraCheckbox = true;
  bool _addNodesCheckbox = true;
  std::vector<std::int32_t> _cachedKeys;
};

}