#include "Application.h"

#include "GLFW/glfw3.h"

#include "../../common/input/KeyInput.h"
#include "../../common/input/MousePosInput.h"
#include "../../common/input/MouseButtonInput.h"

#include <iostream>
#include <string>

static void errorCallback(int error, const char* description)
{
  std::cerr << "GLFW Error code: " << std::to_string(error) << ", description: " << description << std::endl;
}

Application::Application(std::string title)
  : _running(false)
  , _title(std::move(title))
{
  glfwSetErrorCallback(errorCallback);
  if (!glfwInit())
    exit(-1);

  // Create gl window stuff
  initWindowHandle();
}

Application::~Application()
{
  glfwDestroyWindow(_window);
  glfwTerminate();
}

void Application::run()
{
  // Init Main loop stuff
  int frames = 0;
  std::string fps = " fps";

  _running = true;

  // Main loop
  double lastUpdate = glfwGetTime();
  double lastFrameUpdate = glfwGetTime();
  while (_running  && !glfwWindowShouldClose(_window)) {
    // Logic update
    glfwPollEvents();

    double now = glfwGetTime();
    double delta = (now - lastUpdate);
    lastUpdate = now;
    update(delta);

    // Render
    render();

    // Render stop

    frames++;
    double now2 = glfwGetTime();
    if ((now2 - lastFrameUpdate) > 1.0) {
      lastFrameUpdate = now2;
      glfwSetWindowTitle(_window, std::string(_title + " " + std::to_string(frames) + fps).c_str());
      frames = 0;
    }
  }
}

void Application::initWindowHandle()
{
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  _window = glfwCreateWindow(1920, 1080, _title.c_str(), NULL, NULL);

  if (!_window) {
    std::cerr << "Failed to open GLFW window" << std::endl;
    glfwTerminate();
    exit(-2);
  }

  // Set input callbacks
  //glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  //glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetKeyCallback(_window, KeyInput::invoke);
  //glfwSetCursorPosCallback(_window, MousePosInput::invoke);
  MousePosInput::_window = _window;
  glfwSetMouseButtonCallback(_window, MouseButtonInput::invoke);
}