#include "MousePosInput.h"

#include <stdio.h>

double MousePosInput::_x;
double MousePosInput::_y;

GLFWwindow* MousePosInput::_window = nullptr;

void MousePosInput::invoke(GLFWwindow* window, double xPos, double yPos)
{
  _x = xPos;
  _y = yPos;
}