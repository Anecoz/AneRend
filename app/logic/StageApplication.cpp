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
  , _sunDir(glm::vec3(0.7f, -0.7f, 0.7f))
  , _vkRenderer(_window)
  , _camera(glm::vec3(-17.0, 6.0, 4.0), render::ProjectionType::Perspective)
  , _shadowCamera(glm::vec3(-20.0f, 15.0f, -20.0f), render::ProjectionType::Orthogonal)
{

}

StageApplication::~StageApplication()
{
}

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
    _modelId = _vkRenderer.registerRenderable(_testModel._vertices, _testModel._indices, render::STANDARD_INSTANCED_MATERIAL_ID, numInstances * numInstances, dataVec);
  }

  // Do a couple of the big models
  {
    std::size_t numInstances = 20;
    std::vector<glm::mat4> instanceMatrixData;

    for (int x = 0; x < numInstances; ++x)
    for (int y = 0; y < numInstances; ++y) {
      auto mat = glm::translate(glm::mat4(1.0f), glm::vec3(30.0f * x, 0.0f, 30.0f * y));
      instanceMatrixData.emplace_back(std::move(mat));
    }

    std::vector<std::uint8_t> dataVec;
    dataVec.resize(instanceMatrixData.size() * 4 * 4 * 4);
    memcpy(dataVec.data(), instanceMatrixData.data(), instanceMatrixData.size() * 4 * 4 * 4);
    _modelId2 = _vkRenderer.registerRenderable(_testModel2._vertices, _testModel2._indices, render::STANDARD_INSTANCED_MATERIAL_ID, numInstances * numInstances, dataVec);
  }

  printf("Renderable registered! Id1: %lld, id2: %lld\n", _modelId, _modelId2);

  // Set shadow cam somewhere
  _shadowCamera.setViewMatrix(glm::lookAt(glm::vec3(-16.0f, 10.0f, -18.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));

  _lastCamPos = _camera.getPosition();

  return true;
}

void StageApplication::update(double delta)
{
  calculateShadowMatrix();

  if (KeyInput::isKeyClicked(GLFW_KEY_ESCAPE)) {
    _camera._enabled = !_camera._enabled;
  }

  _camera.update(delta);
  _vkRenderer.update(_camera, _shadowCamera, glm::vec4(_sunDir, 0.0f), delta);

  static auto currAngle = 0.0f;
  currAngle += 1.0f * (float)delta;
  auto model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
  //auto model = glm::rotate(glm::mat4(1.0f), currAngle, glm::vec3(0.0f, 1.0f, 0.0f));
  //_vkRenderer.queuePushConstant(_modelId2, 4 * 16, &model);
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
    float dir[2];
    dir[0] = _sunDir.x;
    dir[1] = _sunDir.z;
    if (ImGui::SliderFloat2("Sun dir", dir, 0.0f, 1.0f)) {
      _sunDir.x = dir[0];
      _sunDir.z = dir[1];
    }
    if (ImGui::SliderFloat("Sun height", &_sunDir.y, -1.0f, 0.0f)) {
      _sunDir.x = dir[0];
      _sunDir.z = dir[1];
    }
    ImGui::Checkbox("Shadow debug", &g_DebugShadow);
    ImGui::End();
  }

  _vkRenderer.drawFrame(g_DebugShadow);
}

void StageApplication::calculateShadowMatrix()
{
  //if (glm::distance(_lastCamPos, _camera.getPosition()) < 5.0f) return;

  _lastCamPos = _camera.getPosition();

  // Calculates a "light" projection and view matrix that is aligned to the main cameras frustum.
  // Calculate frustum 8 points in world space
  auto ndcToWorldCam = glm::inverse(_camera.getCombined());
  auto ndcToMainCamView = glm::inverse(_camera.getProjection());

  const double ndcMin = -1.0;
  const double ndcZMin = 0.0;
  const double ndcZMax = 1.0;
  const double ndcMax = 1.0;
  glm::vec3 nearLeftBottom(ndcMin, ndcMax, ndcZMin);
  glm::vec3 farLeftBottom(ndcMin, ndcMax, ndcZMax);
  glm::vec3 farRightBottom(ndcMax, ndcMax, ndcZMax);
  glm::vec3 farRightTop(ndcMax, ndcMin, ndcZMax);
  glm::vec3 farLeftTop(ndcMin, ndcMin, ndcZMax);
  glm::vec3 nearLeftTop(ndcMin, ndcMin, ndcZMin);
  glm::vec3 nearRightBottom(ndcMax, ndcMax, ndcZMin);
  glm::vec3 nearRightTop(ndcMax, ndcMin, ndcZMin);

  std::vector<glm::vec4> cornersWorld;
  glm::vec4 midPoint(0.0, 0.0, 0.0, 0.0);

  glm::vec4 wNearLeftBottom = ndcToWorldCam * glm::vec4(nearLeftBottom, 1.0);
  wNearLeftBottom /= wNearLeftBottom.w;
  glm::vec4 wNearLeftTop = ndcToWorldCam * glm::vec4(nearLeftTop, 1.0);
  wNearLeftTop /= wNearLeftTop.w;
  glm::vec4 wNearRightBottom = ndcToWorldCam * glm::vec4(nearRightBottom, 1.0);
  wNearRightBottom /= wNearRightBottom.w;
  glm::vec4 wNearRightTop = ndcToWorldCam * glm::vec4(nearRightTop, 1.0);
  wNearRightTop /= wNearRightTop.w;

  glm::vec4 tmp;
  const float percentageFar = 1.0f;

  glm::vec4 wFarLeftBottom = ndcToWorldCam * glm::vec4(farLeftBottom, 1.0);
  wFarLeftBottom /= wFarLeftBottom.w;
  tmp = wFarLeftBottom - wNearLeftBottom;
  wFarLeftBottom = wNearLeftBottom + glm::normalize(tmp) * percentageFar * glm::length(tmp);

  glm::vec4 wFarRightBottom = ndcToWorldCam * glm::vec4(farRightBottom, 1.0);
  wFarRightBottom /= wFarRightBottom.w;
  tmp = wFarRightBottom - wNearRightBottom;
  wFarRightBottom = wNearRightBottom + glm::normalize(tmp) * percentageFar * glm::length(tmp);

  glm::vec4 wFarRightTop = ndcToWorldCam * glm::vec4(farRightTop, 1.0);
  wFarRightTop /= wFarRightTop.w;
  tmp = wFarRightTop - wNearRightTop;
  wFarRightTop = wNearRightTop + glm::normalize(tmp) * percentageFar * glm::length(tmp);

  glm::vec4 wFarLeftTop = ndcToWorldCam * glm::vec4(farLeftTop, 1.0);
  wFarLeftTop /= wFarLeftTop.w;
  tmp = wFarLeftTop - wNearLeftTop;
  wFarLeftTop = wNearLeftTop + glm::normalize(tmp) * percentageFar * glm::length(tmp);

  cornersWorld.emplace_back(wNearLeftBottom);
  cornersWorld.emplace_back(wNearLeftTop);
  cornersWorld.emplace_back(wNearRightBottom);
  cornersWorld.emplace_back(wNearRightTop);
  cornersWorld.emplace_back(wFarLeftBottom);
  cornersWorld.emplace_back(wFarRightBottom);
  cornersWorld.emplace_back(wFarRightTop);
  cornersWorld.emplace_back(wFarLeftTop);

  midPoint = wNearLeftBottom + wNearLeftTop + wNearRightBottom + wNearRightTop + wFarLeftBottom + wFarRightBottom + wFarRightTop + wFarLeftTop;
  midPoint /= 8.0;
  
  glm::mat4 lvMatrix = glm::lookAt(glm::vec3(midPoint) - glm::normalize(_sunDir), glm::vec3(midPoint), glm::vec3(0.0, 1.0, 0.0));

  glm::vec4 transf = lvMatrix * cornersWorld[0];
  float minZ = transf.z;
  float maxZ = transf.z;
  float minX = transf.x;
  float maxX = transf.x;
  float minY = transf.y;
  float maxY = transf.y;

  //printf("Corner world pos: %lf %lf %lf\n", cornersWorld[0].x, cornersWorld[0].y, cornersWorld[0].z);
  for (unsigned int i = 1; i < 8; i++) {
    transf = lvMatrix * cornersWorld[i];

    //printf("Corner world pos: %lf %lf %lf\n", cornersWorld[i].x, cornersWorld[i].y, cornersWorld[i].z);

    if (transf.z > maxZ) maxZ = transf.z;
    if (transf.z < minZ) minZ = transf.z;
    if (transf.x > maxX) maxX = transf.x;
    if (transf.x < minX) minX = transf.x;
    if (transf.y > maxY) maxY = transf.y;
    if (transf.y < minY) minY = transf.y;
  }

  //printf("minx, maxx, miny, maxy, minz, maxz: %lf %lf %lf %lf %lf %lf\n", minX, maxX, minY, maxY, minZ, maxZ);
  /*auto nearMinX = (lvMatrix * wNearLeftTop).x;
  auto nearMaxX = (lvMatrix * wNearRightTop).x;
  auto nearMinY = (lvMatrix * wNearLeftBottom).y;
  auto nearMaxY = (lvMatrix * wNearRightTop).y;*/

  glm::mat4 lpMatrix = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

  _shadowCamera.setViewMatrix(lvMatrix);
  _shadowCamera.setProjection(lpMatrix);
}

void StageApplication::notifyFramebufferResized()
{
  _vkRenderer.notifyFramebufferResized();
}