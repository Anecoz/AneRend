#include "StageApplication.h"

#include "../input/KeyInput.h"
#include "../imgui/imgui.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
  auto app = reinterpret_cast<StageApplication*>(glfwGetWindowUserPointer(window));
  app->notifyFramebufferResized();
}

StageApplication::StageApplication(std::string title)
  : Application(std::move(title))
  , _vkRenderer(_window)
  , _camera(glm::vec3(10.0, 10.0, 10.0), render::ProjectionType::Perspective)
{

}

StageApplication::~StageApplication()
{
}

bool StageApplication::init()
{
  glfwSetWindowUserPointer(_window, this);
  glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);

  if (!_vkRenderer.init()) {
    return false;
  }

  if (!_testModel.loadFromObj(std::string(ASSET_PATH) + "models/low_poly_tree.obj",
                              std::string(ASSET_PATH) + "models/")) {
    return false;
  }

  if (!_testModel2.loadFromObj(std::string(ASSET_PATH) + "models/lots_of_models.obj",
                              std::string(ASSET_PATH) + "models/")) {
    return false;
  }

  // Create a bunch of test matrices
  {
    std::size_t numInstances = 100;
    std::vector<glm::mat4> instanceMatrixData;
    instanceMatrixData.reserve(numInstances * numInstances);
    for (std::size_t x = 0; x < numInstances; ++x)
    for (std::size_t y = 0; y < numInstances; ++y) {
      auto trans = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f * x * 6, 0.0f, 1.0f * y * 6));
      auto rot = glm::rotate(glm::mat4(1.0f), glm::radians(float(rand() % 360)), glm::vec3(0.0f, 1.0f, 0.0f));

      auto matrix = trans * rot;

      instanceMatrixData.emplace_back(std::move(matrix));
    }

    std::vector<std::uint8_t> dataVec;
    dataVec.resize(instanceMatrixData.size() * 4 * 4 * 4);
    memcpy(dataVec.data(), instanceMatrixData.data(), instanceMatrixData.size() * 4 * 4 * 4);
    _modelId = _vkRenderer.registerRenderable(_testModel._vertices, _testModel._indices, render::STANDARD_INSTANCED_MATERIAL_ID, numInstances * numInstances, dataVec);
  }

  _modelId2 = _vkRenderer.registerRenderable(_testModel2._vertices, _testModel2._indices, render::STANDARD_MATERIAL_ID);
  printf("Renderable registered! Id1: %lld, id2: %lld\n", _modelId, _modelId2);

  if (_modelId == 0) {
    return false;
  }

  return true;
}

void StageApplication::update(double delta)
{
  if (KeyInput::isKeyClicked(GLFW_KEY_ESCAPE)) {
    _camera._enabled = !_camera._enabled;
  }

  _camera.update(delta);
  _vkRenderer.update(_camera, delta);

  /*{
    static auto currAngle = 0.0f;
    currAngle += 1.0f * (float)delta;
    auto model = glm::rotate(glm::mat4(1.0f), currAngle * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    _vkRenderer.queuePushConstant(_modelId, 4 * 16, &model);
  }*/
  {
    static auto currAngle = 0.0f;
    static bool once = false;
    if (!once) {
      currAngle += 1.0f * (float)delta;
      auto model = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, 0.0f));
      _vkRenderer.queuePushConstant(_modelId2, 4 * 16, &model);
      once = true;
    }
  }
}

void StageApplication::render()
{
  _vkRenderer.prepare();

  ImGui::ShowDemoWindow();

  _vkRenderer.drawFrame();
}

void StageApplication::notifyFramebufferResized()
{
  _vkRenderer.notifyFramebufferResized();
}