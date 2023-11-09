#include "logic/StageApplication.h"

int main()
{
  StageApplication app("Stage");

  if (!app.init()) {
    return -1;
  }

  app.run();
  return 0;
}