#include "KeyInput.h"

bool KeyInput::_keys[512];
std::vector<int> KeyInput::_pressed;

void KeyInput::invoke(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  _keys[key] = action != GLFW_RELEASE;
  if (action == GLFW_RELEASE && std::find(_pressed.begin(), _pressed.end(), key) != _pressed.end()) {
    _pressed.erase(remove(_pressed.begin(), _pressed.end(), key), _pressed.end());
  }
}