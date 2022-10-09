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
  , _camera(glm::vec3(-17.0, 6.0, 4.0), render::ProjectionType::Perspective)
  , _shadowCamera(glm::vec3(-20.0f, 15.0f, -20.0f), render::ProjectionType::Orthogonal)
{

}

StageApplication::~StageApplication()
{
}

static float g_NearPlane = -100.0f;
static float g_FarPlane = 100.0f;
static float g_LeftRight = 10.0f;
static float g_UpDown = 10.0f;
static bool g_DebugShadow = true;

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
    //_modelId = _vkRenderer.registerRenderable(_testModel._vertices, _testModel._indices, render::STANDARD_INSTANCED_MATERIAL_ID, numInstances * numInstances, dataVec);
  }

  _modelId2 = _vkRenderer.registerRenderable(_testModel2._vertices, _testModel2._indices, render::STANDARD_MATERIAL_ID);
  printf("Renderable registered! Id1: %lld, id2: %lld\n", _modelId, _modelId2);

  // Set shadow cam somewhere
  _shadowCamera.setViewMatrix(glm::lookAt(glm::vec3(-16.0f, 10.0f, -18.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));

  return true;
}

void StageApplication::update(double delta)
{
  if (KeyInput::isKeyClicked(GLFW_KEY_ESCAPE)) {
    _camera._enabled = !_camera._enabled;
  }

  _camera.update(delta);
  //_shadowCamera.setViewMatrix(_camera.getCamMatrix());
  _vkRenderer.update(_camera, _shadowCamera, delta);

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
      auto model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
      _vkRenderer.queuePushConstant(_modelId2, 4 * 16, &model);
      once = true;
    }
  }
}

void StageApplication::render()
{
  _vkRenderer.prepare();

  {
    ImGui::Begin("Debug");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("Camera pos %.1f, %.1f, %.1f", _camera.getPosition().x, _camera.getPosition().y, _camera.getPosition().z);
    ImGui::End();
  }
  {
    ImGui::Begin("Parameters");
    if (ImGui::SliderFloat("Near plane", &g_NearPlane, -1000.0f, 1000.0f)) {
      _shadowCamera.setProjection(glm::ortho(-g_LeftRight, g_LeftRight, -g_UpDown, g_UpDown, g_NearPlane, g_FarPlane));
    }
    if (ImGui::SliderFloat("Far plane", &g_FarPlane, -1000.0f, 1000.0f)) {
      _shadowCamera.setProjection(glm::ortho(-g_LeftRight, g_LeftRight, -g_UpDown, g_UpDown, g_NearPlane, g_FarPlane));
    }
    if (ImGui::SliderFloat("Ortho proj left/right", &g_LeftRight, -100.0f, 100.0f)) {
      _shadowCamera.setProjection(glm::ortho(-g_LeftRight, g_LeftRight, -g_UpDown, g_UpDown, g_NearPlane, g_FarPlane));
    }
    if (ImGui::SliderFloat("Ortho proj up/down", &g_UpDown, -100.0f, 100.0f)) {
      _shadowCamera.setProjection(glm::ortho(-g_LeftRight, g_LeftRight, -g_UpDown, g_UpDown, g_NearPlane, g_FarPlane));
    }
    ImGui::Checkbox("Shadow debug", &g_DebugShadow);
    ImGui::End();
  }

  _vkRenderer.drawFrame(g_DebugShadow);
}

void StageApplication::notifyFramebufferResized()
{
  _vkRenderer.notifyFramebufferResized();
}