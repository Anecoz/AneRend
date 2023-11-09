#include "logic/AneditApplication.h"

int main()
{
  AneditApplication app("AnEdit");

  if (!app.init()) {
    return -1;
  }

  app.run();
  return 0;
}