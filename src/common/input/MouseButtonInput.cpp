#include "MouseButtonInput.h"

bool MouseButtonInput::_leftDown = false;
bool MouseButtonInput::_rightDown = false;
std::vector<int> MouseButtonInput::_pressed;
bool MouseButtonInput::_buttons[128];

void MouseButtonInput::invoke(GLFWwindow* window, int button, int action, int mods) {
  _buttons[button] = action != GLFW_RELEASE;

  if (action == GLFW_RELEASE && std::find(_pressed.begin(), _pressed.end(), button) != _pressed.end())
    _pressed.erase(std::remove(_pressed.begin(), _pressed.end(), button), _pressed.end());

  if (button == GLFW_MOUSE_BUTTON_1) {
    if (action == GLFW_PRESS)
      _leftDown = true;
    else if (action == GLFW_RELEASE)
      _leftDown = false;
  }
  else if (button == GLFW_MOUSE_BUTTON_2) {
    if (action == GLFW_PRESS)
      _rightDown = true;
    else if (action == GLFW_RELEASE)
      _rightDown = false;
  }
}