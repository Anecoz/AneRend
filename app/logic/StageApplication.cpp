#include "StageApplication.h"

#include "../input/KeyInput.h"
#include "../imgui/imgui.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <map>

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
  auto app = reinterpret_cast<StageApplication*>(glfwGetWindowUserPointer(window));
  app->notifyFramebufferResized();
}

StageApplication::StageApplication(std::string title)
  : Application(std::move(title))
  , _sunDir(glm::vec3(0.7f, -0.7f, 0.7f))
  , _camera(glm::vec3(-17.0, 6.0, 4.0), render::ProjectionType::Perspective)
  , _vkRenderer(_window, _camera)
  , _windDir(glm::vec2(-1.0, 0.0))
  , _shadowCamera(glm::vec3(-20.0f, 15.0f, -20.0f), render::ProjectionType::Orthogonal)
{

}

StageApplication::~StageApplication()
{
}

static bool g_DebugShadow = true;
static bool g_LockFrustumCulling = false;
static bool g_PP = true;

bool StageApplication::init()
{
  glfwSetWindowUserPointer(_window, this);
  glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);

  render::Model testModel;
  render::Model testModel2;
  render::Model testModel3;
  render::Model testModel4;
  render::Model testModel5;
  render::Model testModel6;
  render::Model testModel7;
  render::Model testModel8;

  if (!_vkRenderer.init()) {
    return false;
  }

  if (!testModel.loadFromObj(std::string(ASSET_PATH) + "models/low_poly_tree.obj",
                              std::string(ASSET_PATH) + "models/")) {
    return false;
  }

  if (!testModel2.loadFromObj(std::string(ASSET_PATH) + "models/lots_of_models.obj",
                              std::string(ASSET_PATH) + "models/")) {
    return false;
  }

  if (!testModel3.loadFromGLTF(std::string(ASSET_PATH) + "models/lantern_gltf/Lantern.glb")) {
    return false;
  }

  if (!testModel4.loadFromObj(std::string(ASSET_PATH) + "models/old_lantern_pbr/lantern_obj.obj",
    std::string(ASSET_PATH) + "models/old_lantern_pbr/",
    std::string(ASSET_PATH) + "models/old_lantern_pbr/textures/lantern_Metallic.jpg",
    std::string(ASSET_PATH) + "models/old_lantern_pbr/textures/lantern_Roughness.jpg",
    std::string(ASSET_PATH) + "models/old_lantern_pbr/textures/lantern_Normal_OpenGL.jpg",
    std::string(ASSET_PATH) + "models/old_lantern_pbr/textures/lantern_Base_Color.jpg")) {
    return false;
  }

  if (!testModel5.loadFromObj(std::string(ASSET_PATH) + "models/gas_tank/GasTank.obj",
    std::string(ASSET_PATH) + "models/gas_tank/",
    std::string(ASSET_PATH) + "models/gas_tank/GasTank_Metallic.png",
    std::string(ASSET_PATH) + "models/gas_tank/GasTank_Roughness.png",
    std::string(ASSET_PATH) + "models/gas_tank/GasTank_Normal.png",
    std::string(ASSET_PATH) + "models/gas_tank/GasTank_Base_Color.png")) {
    return false;
  }

  if (!testModel6.loadFromGLTF(std::string(ASSET_PATH) + "models/damaged_helmet_gltf/DamagedHelmet.glb")) {
    return false;
  }

  if (!testModel7.loadFromGLTF(std::string(ASSET_PATH) + "models/sponza-gltf-pbr/sponza.glb")) {
    return false;
  }

  if (!testModel8.loadFromGLTF(std::string(ASSET_PATH) + "models/metal_rough_spheres_gltf/MetalRoughSpheres.glb")) {
    return false;
  }

  auto testMin = testModel7._min;
  auto testMax = testModel7._max;

  _meshId = _vkRenderer.registerModel(std::move(testModel));
  _meshId2 = _vkRenderer.registerModel(std::move(testModel2));
  _meshId3 = _vkRenderer.registerModel(std::move(testModel3));
  _meshId4 = _vkRenderer.registerModel(std::move(testModel4));
  _meshId5 = _vkRenderer.registerModel(std::move(testModel5));
  _meshId6 = _vkRenderer.registerModel(std::move(testModel6));
  _meshId7 = _vkRenderer.registerModel(std::move(testModel7));
  _meshId8 = _vkRenderer.registerModel(std::move(testModel8));

  // Create a bunch of test matrices
  // Trees
  {
    std::size_t numInstances = 100;
    for (std::size_t x = 0; x < numInstances; ++x)
    for (std::size_t y = 0; y < numInstances; ++y) {
      auto trans = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f * x * 6, 0.0f, 1.0f * y * 6));
      auto rot = glm::rotate(glm::mat4(1.0f), glm::radians(float(rand() % 360)), glm::vec3(0.0f, 1.0f, 0.0f));

      auto matrix = trans * rot;
      glm::vec3 sphereBoundCenter(0.0f, 4.0f, 0.0f);
      //sphereBoundCenter = trans * sphereBoundCenter;

      auto ret = _vkRenderer.registerRenderable(
        _meshId,
        matrix,
        sphereBoundCenter,
        5.0f,
        false);

      if (ret == 0) {
        break;
      }
    }
  }

  // Do a couple of the big models
  // Ground level
  {
    std::size_t numInstances = 20;

    for (int x = 0; x < numInstances; ++x)
    for (int y = 0; y < numInstances; ++y) {
      auto mat = glm::translate(glm::mat4(1.0f), glm::vec3(30.0f * x, 0.0f, 30.0f * y));
      _vkRenderer.registerRenderable(_meshId2, mat, glm::vec3(0.0f), 50.0f);
    }
  }

  // Lantern GLTF
  {
    std::size_t numInstances = 20;

    for (int x = 0; x < numInstances; ++x)
      for (int y = 0; y < numInstances; ++y) {
        auto mat = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f * x, 0.0f, 5.0f * y));
        auto scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.2f));
        _vkRenderer.registerRenderable(_meshId3, mat * scale, glm::vec3(1.0f), 10.0f);
      }
  }
  {
    std::size_t numInstances = 0;

    for (int x = 0; x < numInstances; ++x)
      for (int y = 0; y < numInstances; ++y) {
        auto mat = glm::translate(glm::mat4(1.0f), glm::vec3(16.0f * x, 1.0f, 8.0f * y));
        auto scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f, 0.01f, 0.01f));
        auto rot = glm::rotate(glm::mat4(1.0f), glm::radians(float(rand() % 360)), glm::vec3(0.0f, 1.0f, 0.0f));
        _vkRenderer.registerRenderable(_meshId4, mat * rot * scale, glm::vec3(0.0f, 0.3f, 0.0f), .7f);
      }
  }
  {
    std::size_t numInstances = 0;

    for (int x = 0; x < numInstances; ++x)
      for (int y = 0; y < numInstances; ++y) {
        auto mat = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f * x, 1.0f, 5.0f * y));
        auto scale = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
        auto rot = glm::rotate(glm::mat4(1.0f), glm::radians(float(rand() % 360)), glm::vec3(0.0f, 1.0f, 0.0f));
        _vkRenderer.registerRenderable(_meshId5, mat * rot * scale, glm::vec3(0.0f), 5.0f);
      }
  }
  // Damaged helmet
  {
    std::size_t numInstances = 20;

    for (int x = 0; x < numInstances; ++x)
      for (int y = 0; y < numInstances; ++y) {
        auto mat = glm::translate(glm::mat4(1.0f), glm::vec3(12.0f * x, 4.0f, 17.0f * y));
        //auto scale = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
        auto rot = glm::rotate(glm::mat4(1.0f), glm::radians(float(rand() % 360)), glm::vec3(0.0f, 1.0f, 0.0f));
        rot = rot * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        _vkRenderer.registerRenderable(_meshId6, mat * rot, glm::vec3(0.0f), 5.0f);
      }
  }
  // Sponza GLTF
  {
    std::size_t numInstances = 0;

    _sponzaPos = glm::vec3(16.0f, -1.0f, 16.0f);

    glm::vec3 sphereCenter{ 0.0f };
    float radius = std::max(std::abs(testMax.z - testMin.z), std::max(std::abs(testMax.x - testMin.x), std::abs(testMax.y - testMin.y)));
    radius /= 2.0f;

    float scale = 0.01f;
    radius *= scale;

    for (int x = 0; x < numInstances; ++x)
      for (int y = 0; y < numInstances; ++y) {
        auto mat = glm::translate(glm::mat4(1.0f), _sponzaPos + glm::vec3(12.0f * x, 0.0f, 17.0f * y));
        _sponzaScale = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
        _sponzaId = _vkRenderer.registerRenderable(_meshId7, mat * _sponzaScale, sphereCenter, radius);
      }
  }
  // Metal rough spheres
  {
    std::size_t numInstances = 0;

    for (int x = 0; x < numInstances; ++x)
      for (int y = 0; y < numInstances; ++y) {
        auto trans = glm::translate(glm::mat4(1.0f), glm::vec3(12.0f * x, 10.0f, 17.0f * y));
        auto scale = glm::scale(glm::mat4(1.0f), glm::vec3(.4f));
        auto rot = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        //rot = rot * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        _vkRenderer.registerRenderable(_meshId8, trans * rot * scale, glm::vec3(0.0f), 50.0f);
      }
  }

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

  //_windSystem.update(delta);
  _windSystem.setWindDir(glm::normalize(_windDir));

  if (_sponzaMoved) {
    auto trans = glm::translate(glm::mat4(1.0f), _sponzaPos);
    _vkRenderer.updateRenderableTransform(_sponzaId, trans * _sponzaScale);
    _sponzaMoved = false;
  }

  _vkRenderer.update(
    _camera,
    _shadowCamera,
    glm::vec4(_sunDir, 0.0f),
    delta, glfwGetTime(),
    g_LockFrustumCulling,
    _renderOptions,
    _renderDebugOptions,
    _windSystem.getCurrentWindMap());
}

void StageApplication::render()
{
  _vkRenderer.prepare();

  ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

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
    if (ImGui::SliderFloat2("Sun dir", dir, -1.0f, 1.0f)) {
      _sunDir.x = dir[0];
      _sunDir.z = dir[1];
    }
    if (ImGui::SliderFloat("Sun height", &_sunDir.y, -1.0f, 1.0f)) {
      _sunDir.x = dir[0];
      _sunDir.z = dir[1];
    }
    if (ImGui::SliderFloat("Sponza X", &_sponzaPos.x, -100.0f, 100.0f)) {
      _sponzaMoved = true;
    }

    static char debugResStr[128] = "ReflectTex";
    if (ImGui::InputText("Debug view resource", debugResStr, IM_ARRAYSIZE(debugResStr), ImGuiInputTextFlags_EnterReturnsTrue)) {
      _renderDebugOptions.debugViewResource = std::string(debugResStr);
    }
    ImGui::InputInt("Debug view mip", &_renderDebugOptions.debugMip);
    /*ImGui::Slider2DFloat("Sun dir", &_sunDir.x, &_sunDir.z, -1.0f, 1.0f, -1.0f, 1.0f);
    ImGui::Slider2DFloat("Wind dir", &_windDir.x, &_windDir.y, -1.0f, 1.0f, -1.0f, 1.0f);*/
    ImGui::Checkbox("Texture Debug View", &_renderDebugOptions.debugView);
    //ImGui::Checkbox("Apply post processing", &g_PP);
    ImGui::End();
  }
  {
    ImGui::Begin("Render Options");
    ImGui::Checkbox("SSAO", &_renderOptions.ssao);
    ImGui::Checkbox("FXAA", &_renderOptions.fxaa);
    ImGui::Checkbox("Shadow map", &_renderOptions.directionalShadows);
    ImGui::Checkbox("Ray traced shadows", &_renderOptions.raytracedShadows);
    ImGui::Checkbox("DDGI", &_renderOptions.ddgiEnabled);
    ImGui::Checkbox("Multi-bounce DDGI", &_renderOptions.multiBounceDdgiEnabled);
    ImGui::Checkbox("Specular GI", &_renderOptions.specularGiEnabled);
    ImGui::Checkbox("Visualize bounding spheres", &_renderOptions.visualizeBoundingSpheres);
    ImGui::Checkbox("Debug probes", &_renderOptions.probesDebug);
    ImGui::Checkbox("Hack", &_renderOptions.hack);
    ImGui::SliderFloat("Sun intensity", &_renderOptions.sunIntensity, 0.0f, 200.0f);
    ImGui::SliderFloat("Sky intensity", &_renderOptions.skyIntensity, 0.0f, 20.0f);
    ImGui::Checkbox("Lock frustum culling", &g_LockFrustumCulling);
    ImGui::End();
  }
  {
    auto timers = _vkRenderer.getPerFrameTimers();

    // Group them
    std::vector<std::string> groups;
    //std::map<std::string, std::vector<render::PerFrameTimer>> groups{};
    for (auto& timer : timers) {
      if (std::find(groups.begin(), groups.end(), timer._group) == groups.end()) {
        groups.emplace_back(timer._group);
      }
    }

    ImGui::Begin("Per frame timers");

    float sum = 0.0f;
    if (ImGui::BeginTable("table3", 2)) {
      for (auto& group : groups) {
        float avg10 = 0.0f;
        for (auto& timer : timers) {
          if (timer._group == group) {
            avg10 += timer._avg10;
          }
        }
        sum += avg10;

        ImGui::TableNextColumn();
        ImGui::Text("%s", group.c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%f", avg10);
      }
      ImGui::EndTable();
    }

    /*for (auto& timer : timers) {
      sum += timer._avg10;
      ImGui::Text("%s: %f", timer._name.c_str(), timer._avg10);

      ImGui::PlotLines("", timer._buf.data(), 1000);
    }*/
    ImGui::Text("Sum: %f", sum);

    ImGui::End();
  }

  _vkRenderer.drawFrame(g_PP, g_DebugShadow);
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
  midPoint /= 8.0f;
  
  glm::mat4 lvMatrix = glm::lookAt(glm::vec3(midPoint) - glm::normalize(_sunDir), glm::vec3(midPoint), glm::vec3(0.0, 1.0, 0.0));

  glm::vec4 transf = lvMatrix * cornersWorld[0];
  float minZ = transf.z;
  float maxZ = transf.z;
  float minX = transf.x;
  float maxX = transf.x;
  float minY = transf.y;
  float maxY = transf.y;

  for (unsigned int i = 1; i < 8; i++) {
    transf = lvMatrix * cornersWorld[i];

    if (transf.z > maxZ) maxZ = transf.z;
    if (transf.z < minZ) minZ = transf.z;
    if (transf.x > maxX) maxX = transf.x;
    if (transf.x < minX) minX = transf.x;
    if (transf.y > maxY) maxY = transf.y;
    if (transf.y < minY) minY = transf.y;
  }

  glm::mat4 lpMatrix = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

  _shadowCamera.setViewMatrix(lvMatrix);
  _shadowCamera.setProjection(lpMatrix, minZ, maxZ);
}

void StageApplication::notifyFramebufferResized()
{
  _vkRenderer.notifyFramebufferResized();
}