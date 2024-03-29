#pragma once

#include "../IGUI.h"

#include "../../tool/Brush.h"

namespace gui {

class EditTerrainGUI : public IGUI
{
public:
  EditTerrainGUI();
  ~EditTerrainGUI();

  void immediateDraw(logic::AneditContext* c) override final;
private:
  bool _paintingMat = false;
  bool _paintingVeg = false;
  bool _eraser = false;
  int _paintMatIndex = 0;
  float _paintOpacity = 1.0f;
  tool::Brush _brush;
};

}