#pragma once

namespace logic { struct AneditContext; }

namespace gui
{

class IGUI
{
public:
  IGUI() {}
  virtual ~IGUI() {}

  virtual void immediateDraw(logic::AneditContext*) = 0;
};

}