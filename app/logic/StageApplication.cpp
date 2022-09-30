#include "StageApplication.h"

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
  auto app = reinterpret_cast<StageApplication*>(glfwGetWindowUserPointer(window));
  app->notifyFramebufferResized();
}

StageApplication::StageApplication(std::string title)
  : Application(std::move(title))
  , _vkRenderer(_window)
{

}

StageApplication::~StageApplication()
{
}

bool StageApplication::init()
{
  glfwSetWindowUserPointer(_window, this);
  glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);

  return _vkRenderer.init();
}

void StageApplication::update(double delta)
{
  
}

void StageApplication::render()
{
  _vkRenderer.drawFrame();
}

void StageApplication::notifyFramebufferResized()
{
  _vkRenderer.notifyFramebufferResized();
}