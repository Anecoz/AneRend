#include "AneditApplication.h"

#include "../../common/input/KeyInput.h"
#include "../../common/input/MousePosInput.h"
#include <util/GLTFLoader.h>

#include "../gui/SceneAssetGUI.h"
#include "../gui/SceneListGUI.h"
#include "../gui/EditRenderableGUI.h"
#include "../gui/EditMaterialGUI.h"
#include "../gui/EditPrefabGUI.h"
#include "../gui/EditAnimatorGUI.h"
#include "../gui/EditLightGUI.h"

#include <imgui.h>
#include <nfd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <random>
#include <map>

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
  auto app = reinterpret_cast<AneditApplication*>(glfwGetWindowUserPointer(window));
  app->notifyFramebufferResized();
}

AneditApplication::AneditApplication(std::string title)
  : Application(std::move(title))
  , _sunDir(glm::vec3(0.7f, -0.7f, 0.7f))
  , _camera(glm::vec3(-17.0, 6.0, 4.0), render::ProjectionType::Perspective)
  , _vkRenderer(_window, _camera)
  , _windDir(glm::vec2(-1.0, 0.0))
  , _shadowCamera(glm::vec3(-20.0f, 15.0f, -20.0f), render::ProjectionType::Orthogonal)
  , _scene()
  , _scenePager(&_vkRenderer)
{
  _scenePager.setScene(&_scene);

  auto cd = std::filesystem::current_path();
  _scenePath = cd/"scene.anerend";
}

AneditApplication::~AneditApplication()
{
  for (auto gui : _guis) {
    delete gui;
  }

  _config._lastCamPos = _camera.getPosition();
  auto p = std::filesystem::current_path() / "anedit.conf";
  _config.saveToPath(std::move(p));

  NFD_Quit();
}

static bool g_LockFrustumCulling = false;

bool AneditApplication::init()
{
  glfwSetWindowUserPointer(_window, this);
  glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);

  if (!_vkRenderer.init()) {
    return false;
  }

  // Init nfd
  NFD_Init();

  // Get config
  auto p = std::filesystem::current_path()/"anedit.conf";
  _config.readFromPath(std::move(p));

  if (!_config._scenePath.empty()) {
    _scenePath = _config._scenePath;
    loadSceneFrom(_scenePath);
  }

  _camera.setPosition(_config._lastCamPos);

  setupGuis();

  // Set shadow cam somewhere
  _shadowCamera.setViewMatrix(glm::lookAt(glm::vec3(-16.0f, 10.0f, -18.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));

  _lastCamPos = _camera.getPosition();

  return true;
}

void AneditApplication::update(double delta)
{
  calculateShadowMatrix();

  if (KeyInput::isKeyClicked(GLFW_KEY_ESCAPE)) {
    _camera.enable(!_camera.enabled());
  }

  if (_sceneFut.valid() && _sceneFut.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    auto dat = _sceneFut.get();

    if (dat._scene != nullptr) {
      // Update the scene
      auto scenePtr = dat._scene.get();
      _scene = std::move(*scenePtr);
      _scenePager.setScene(&_scene);
      scenePtr = nullptr;
    }
  }

  // Check if we have a GLTF model that is being loaded
  if (_gltfImporter.hasResult()) {
    auto gltfData = _gltfImporter.takeResult();
    addGltfDataToScene(std::move(gltfData));
  }

  _camera.update(delta);
  _scenePager.update(_camera.getPosition());
  _scene.resetEvents();

  //_windSystem.update(delta);
  _windSystem.setWindDir(glm::normalize(_windDir));

  _vkRenderer.update(
    _camera,
    _shadowCamera,
    glm::vec4(_sunDir, 0.0f),
    delta, glfwGetTime(),
    g_LockFrustumCulling,
    _renderOptions,
    _renderDebugOptions);
    //_windSystem.getCurrentWindMap());
}

void AneditApplication::render()
{
  _vkRenderer.prepare();

  ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

  {
    ImGui::Begin("Debug");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("Camera pos %.1f, %.1f, %.1f", _camera.getPosition().x, _camera.getPosition().y, _camera.getPosition().z);
    if (ImGui::Button("Teleport to origin")) {
      _camera.setPosition({ 0.0f, 0.0f, 0.0f });
    }
    ImGui::End();
  }

  for (auto gui : _guis) {
    gui->immediateDraw(this);
  }

  oldUI();

  _vkRenderer.drawFrame();
}

void AneditApplication::setupGuis()
{
  _guis.emplace_back(new gui::SceneAssetGUI());
  _guis.emplace_back(new gui::SceneListGUI());
  _guis.emplace_back(new gui::EditRenderableGUI());
  _guis.emplace_back(new gui::EditMaterialGUI());
  _guis.emplace_back(new gui::EditPrefabGUI());
  _guis.emplace_back(new gui::EditAnimatorGUI());
  _guis.emplace_back(new gui::EditLightGUI());
}

void AneditApplication::updateConfig()
{
  _config._scenePath = _scenePath;
}

void AneditApplication::addGltfDataToScene(std::unique_ptr<logic::LoadedGLTFData> data)
{
  _scene.addModel(std::move(data->_model));
  _scene.addPrefab(std::move(data->_prefab));

  if (data->_skeleton) {
    _scene.addSkeleton(std::move(data->_skeleton));
  }

  for (auto& anim : data->_animations) {
    _scene.addAnimation(std::move(anim));
  }

  for (auto& tex : data->_textures) {
    _scene.addTexture(std::move(tex));
  }

  for (auto& mat : data->_materials) {
    _scene.addMaterial(std::move(mat));
  }
}

void AneditApplication::oldUI()
{
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

    static char debugResStr[128] = "ReflectTex";
    if (ImGui::InputText("Debug view resource", debugResStr, IM_ARRAYSIZE(debugResStr), ImGuiInputTextFlags_EnterReturnsTrue)) {
      _renderDebugOptions.debugViewResource = std::string(debugResStr);
    }
    ImGui::InputInt("Debug view mip", &_renderDebugOptions.debugMip);
    ImGui::Checkbox("Texture Debug View", &_renderDebugOptions.debugView);

    // Test bake
    static int idxX = 0;
    static int idxY = 0;
    ImGui::InputInt("Bake tile idx X", &idxX);
    ImGui::InputInt("Bake tile idx Z", &idxY);
    if (ImGui::Checkbox("Test bake", &_baking)) {
      if (_baking) {
        render::scene::TileIndex idx(idxX, idxY);
        _vkRenderer.startBakeDDGI(idx);
      }
      else {
        auto origPos = _vkRenderer.stopBake(
          [this](render::asset::Texture bakedDDGITex) {
            // Add to the scene
            auto id = _scene.addTexture(std::move(bakedDDGITex));

            render::scene::TileIndex idx(idxX, idxY);
            _scene.setDDGIAtlas(id, idx);
          });
        _camera.setPosition(origPos);
      }
    }

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
    ImGui::Checkbox("SS Probes GI", &_renderOptions.screenspaceProbes);
    ImGui::Checkbox("Visualize bounding spheres", &_renderOptions.visualizeBoundingSpheres);
    ImGui::Checkbox("Debug probes", &_renderOptions.probesDebug);
    ImGui::Checkbox("Hack", &_renderOptions.hack);
    ImGui::SliderFloat("Sun intensity", &_renderOptions.sunIntensity, 0.0f, 200.0f);
    ImGui::SliderFloat("Sky intensity", &_renderOptions.skyIntensity, 0.0f, 20.0f);
    ImGui::SliderFloat("Exposure", &_renderOptions.exposure, 0.0f, 5.0f);
    ImGui::Checkbox("Lock frustum culling", &g_LockFrustumCulling);
    ImGui::End();
  }
  {
    auto timers = _vkRenderer.getPerFrameTimers();

    // Group them
    std::vector<std::string> groups;
    for (auto& timer : timers) {
      if (std::find(groups.begin(), groups.end(), timer._group) == groups.end()) {
        groups.emplace_back(timer._group);
      }
    }

    ImGui::Begin("Per frame timers");

    double sum = 0.0;
    if (ImGui::BeginTable("table3", 2)) {
      for (auto& group : groups) {
        double avg10 = 0.0;
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
    ImGui::Text("Sum: %lf", sum);

    ImGui::End();
  }
}

void AneditApplication::calculateShadowMatrix()
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

void AneditApplication::notifyFramebufferResized()
{
  _vkRenderer.notifyFramebufferResized();
}

render::scene::Scene& AneditApplication::scene()
{
  return _scene;
}

void AneditApplication::serializeScene()
{
  _scene.serializeAsync(_scenePath);
}

void AneditApplication::loadSceneFrom(std::filesystem::path p)
{
  _sceneFut = _scene.deserializeAsync(p);
}

void AneditApplication::setScenePath(std::filesystem::path p)
{
  _scenePath = p;
  updateConfig();
}

void AneditApplication::startLoadGLTF(std::filesystem::path p)
{
  _gltfImporter.startLoad(p);
}

void AneditApplication::spawnFromPrefabAtMouse(const util::Uuid& prefab)
{
  _vkRenderer.requestWorldPosition(MousePosInput::getPosition(),
    [prefab, this](glm::vec3 worldPos) {
      auto* p = _scene.getPrefab(prefab);
      render::asset::Renderable rend{};
      rend._materials = p->_materials;
      rend._model = p->_model;
      rend._skeleton = p->_skeleton;
      rend._name = p->_name.empty() ? "" : p->_name;

      rend._boundingSphere = p->_boundingSphere;
      rend._visible = true;
      rend._tint = p->_tint;

      // Replace translation part of prefab transform with worldPos
      glm::mat4 pMat = p->_transform;
      auto& temp = pMat[3];
      temp.x = worldPos.x;
      temp.y = worldPos.y;
      temp.z = worldPos.z;
      pMat[3] = temp;
      rend._transform = std::move(pMat);

      // Also select it
      _selectedRenderable = rend._id;
      _latestSelection = rend._id;
      _scene.addRenderable(std::move(rend));
    });
}

util::Uuid& AneditApplication::latestSelection()
{
  return _latestSelection;
}

util::Uuid& AneditApplication::selectedRenderable()
{
  return _selectedRenderable;
}

util::Uuid& AneditApplication::selectedMaterial()
{
  return _selectedMaterial;
}

util::Uuid& AneditApplication::selectedPrefab()
{
  return _selectedPrefab;
}

util::Uuid& AneditApplication::selectedAnimator()
{
  return _selectedAnimator;
}

util::Uuid& AneditApplication::selectedLight()
{
  return _selectedLight;
}

render::Camera& AneditApplication::camera()
{
  return _camera;
}
