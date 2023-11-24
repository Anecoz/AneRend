#pragma once

#include "IGUI.h"

#include <util/Uuid.h>

namespace gui {

class EditAnimatorGUI : public IGUI
{
public:
  EditAnimatorGUI();
  ~EditAnimatorGUI();

  void immediateDraw(logic::AneditContext* c) override final;
private:
  bool chooseSkeleton(logic::AneditContext* c, util::Uuid& uuidOut);
  bool chooseAnimation(logic::AneditContext* c, util::Uuid& uuidOut);

  util::Uuid _selectedSkeleton;
  util::Uuid _selectedAnimation;
};

}