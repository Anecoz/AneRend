#include "AneditApplication.h"

#include "../../common/input/KeyInput.h"
#include "../../common/input/MousePosInput.h"
#include <util/GLTFLoader.h>

#include "../gui/SceneAssetGUI.h"
#include "../gui/SceneListGUI.h"
#include "../gui/EditRenderableGUI.h"

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

static bool g_DebugShadow = true;
static bool g_LockFrustumCulling = false;
static bool g_PP = true;

namespace {

bool loadGLTF(
  render::scene::Scene& scene, 
  const std::string& path, 
  util::Uuid& modelIdOut, 
  std::vector<util::Uuid>& materialIdsOut,
  util::Uuid& skeletonIdOut,
  std::vector<util::Uuid>& animationIdsOut)
{
  materialIdsOut.clear();
  animationIdsOut.clear();

  render::asset::Model model{};
  render::anim::Skeleton skeleton{};
  render::asset::Prefab prefab{};
  std::vector<render::anim::Animation> animations;
  std::vector<render::asset::Material> materials;
  std::vector<int> materialIndices;
  if (!util::GLTFLoader::loadFromFile(path, prefab, model, materials, materialIndices, skeleton, animations)) {
    printf("Could not load model (%s)!\n", path.c_str());
    return false;
  }

  // Model and meshes
  modelIdOut = scene.addModel(std::move(model));

  // Materials
  std::vector<util::Uuid> materialIds;
  for (auto& mat : materials) {
    materialIds.emplace_back(scene.addMaterial(std::move(mat)));
  }

  for (auto& idx : materialIndices) {
    materialIdsOut.push_back(materialIds[idx]);
  }

  // Skeleton
  if (skeleton) {
    skeletonIdOut = scene.addSkeleton(std::move(skeleton));
  }

  // Animations
  if (!animations.empty()) {
    for (auto& anim : animations) {
      animationIdsOut.push_back(scene.addAnimation(std::move(anim)));
    }
  }

  if (prefab._id) {
    scene.addPrefab(std::move(prefab));
  }

  return true;
}

util::Uuid addRenderableRandom(
  render::scene::Scene& scene, 
  util::Uuid modelId, 
  std::vector<util::Uuid>& materials, 
  float scale,
  float boundingSphereRadius,
  glm::mat4 rot = glm::mat4(1.0f),
  util::Uuid skeleId = util::Uuid(),
  render::asset::Renderable* renderableOut = nullptr)
{
  static float xOff = 0.0f;
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_real_distribution<> transOff(-5.0, 5.0);
  std::uniform_real_distribution<> rotOff(-180.0, 180.0);

  render::asset::Renderable renderable{};

  renderable._materials = materials;
  renderable._boundingSphere = glm::vec4(.0f, .0f, .0f, boundingSphereRadius);
  renderable._model = modelId;
  renderable._visible = true;
  //auto mat = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f + (float)transOff(rng), 0.0f, 5.0f + (float)transOff(rng)));
  auto trans = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f + xOff, 0.0f, 5.0f));
  //auto rot = glm::rotate(glm::mat4(1.0f), glm::radians((float)rotOff(rng)), glm::vec3(0.0f, 1.0f, 0.0f));
  auto scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
  renderable._transform = trans * rot * scaleMat;

  if (skeleId) {
    renderable._skeleton = skeleId;
  }

  xOff = xOff + 4.0f;

  if (renderableOut) {
    *renderableOut = renderable;
  }

  auto idCopy = scene.addRenderable(std::move(renderable));

  if (renderableOut) {
    renderableOut->_id = idCopy;
  }

  return idCopy;
}

void removeModel(render::VulkanRenderer& renderer, util::Uuid id, std::vector<util::Uuid>& materials)
{
  render::AssetUpdate upd{};
  upd._removedModels.emplace_back(id);
  upd._removedMaterials = materials;
  renderer.assetUpdate(std::move(upd));
}

void removeRenderable(render::VulkanRenderer& renderer, util::Uuid id)
{
  render::AssetUpdate upd{};
  upd._removedRenderables.emplace_back(id);
  renderer.assetUpdate(std::move(upd));
}

}

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

    // Update the scene
    auto scenePtr = dat._scene.get();
    _scene = std::move(*scenePtr);
    _scenePager.setScene(&_scene);
    scenePtr = nullptr;
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
    ImGui::End();
  }

  for (auto gui : _guis) {
    gui->immediateDraw(this);
  }

  oldUI();

  ImGui::End();

  _vkRenderer.drawFrame(g_PP, g_DebugShadow);
}

void AneditApplication::setupGuis()
{
  _guis.emplace_back(new gui::SceneAssetGUI());
  _guis.emplace_back(new gui::SceneListGUI());
  _guis.emplace_back(new gui::EditRenderableGUI());
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
    /*ImGui::Slider2DFloat("Sun dir", &_sunDir.x, &_sunDir.z, -1.0f, 1.0f, -1.0f, 1.0f);
    ImGui::Slider2DFloat("Wind dir", &_windDir.x, &_windDir.y, -1.0f, 1.0f, -1.0f, 1.0f);*/
    ImGui::Checkbox("Texture Debug View", &_renderDebugOptions.debugView);
    //ImGui::Checkbox("Apply post processing", &g_PP);
    if (ImGui::Button("Serialize scene")) {
      _scene.serializeAsync(_scenePath);
    }
    if (ImGui::Button("Deserialize scene")) {
      _sceneFut = _scene.deserializeAsync(_scenePath);
    }
    if (ImGui::Button("Spawn lantern")) {
      _rends.emplace_back(addRenderableRandom(_scene, _lanternModelId, _lanternMaterials, 0.2f, 5.0f));
    }
    if (ImGui::Button("Spawn sponza")) {
      _rends.emplace_back(addRenderableRandom(_scene, _sponzaModelId, _sponzaMaterialIds, 0.01f, 40.0f));
    }
    if (ImGui::Button("Spawn brainstem")) {
      _rends.emplace_back(addRenderableRandom(
        _scene,
        _brainstemModelId,
        _brainstemMaterials,
        1.0f,
        2.0f,
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        _brainstemSkelId));
    }
    if (ImGui::Button("Spawn shrek")) {
      _rends.emplace_back(addRenderableRandom(
        _scene,
        _shrekModelId,
        _shrekMaterials,
        0.001f,
        2.0f,
        glm::mat4(1.0f),
        _shrekSkeleId));
    }
    if (ImGui::Button("Spawn fox")) {
      _rends.emplace_back(addRenderableRandom(
        _scene,
        _foxModelId,
        _foxMaterials,
        .02f,
        2.0f,
        glm::mat4(1.0f),
        _foxSkeleId,
        &_cachedFoxRenderable));
    }
    if (ImGui::Button("Add fox animator")) {
      _foxAnimator._animId = _foxAnims[0];
      _foxAnimator._skeleId = _foxSkeleId;
      _foxAnimator._state = render::asset::Animator::State::Playing;

      auto animatorCopy = _foxAnimator;
      _foxAnimator._id = _scene.addAnimator(std::move(animatorCopy));
    }
    if (ImGui::Button("Change fox animation")) {
      _foxAnimIdx = (_foxAnimIdx + 1) % (int)_foxAnims.size();
      _foxAnimator._animId = _foxAnims[_foxAnimIdx];

      _scene.updateAnimator(_foxAnimator);
    }
    if (ImGui::SliderFloat("Fox playback speed", &_foxAnimator._playbackMultiplier, 0.0f, 5.0f)) {
      _scene.updateAnimator(_foxAnimator);
    }
    if (ImGui::Button("Fox pause")) {
      _foxAnimator._state = render::asset::Animator::State::Paused;
      _scene.updateAnimator(_foxAnimator);
    }
    if (ImGui::Button("Fox play")) {
      _foxAnimator._state = render::asset::Animator::State::Playing;
      _scene.updateAnimator(_foxAnimator);
    }
    if (ImGui::Button("Fox stop")) {
      _foxAnimator._state = render::asset::Animator::State::Stopped;
      _scene.updateAnimator(_foxAnimator);
    }
    if (ImGui::Button("Load sponza")) {
      loadGLTF(_scene, std::string(ASSET_PATH) + "models/sponza-gltf-pbr/sponza.glb", _sponzaModelId, _sponzaMaterialIds, _dummySkele, _dummyAnimations);
    }
    if (ImGui::Button("Load lantern")) {
      loadGLTF(_scene, std::string(ASSET_PATH) + "models/lantern_gltf/Lantern.glb", _lanternModelId, _lanternMaterials, _dummySkele, _dummyAnimations);
    }
    if (ImGui::Button("Load shrek")) {
      std::vector<util::Uuid> animIds;
      loadGLTF(
        _scene,
        std::string(ASSET_PATH) + "models/shrek_dancing_gltf/shrek_dancing.glb",
        _shrekModelId,
        _shrekMaterials,
        _shrekSkeleId,
        animIds);
      _shrekAnimId = animIds[0];
    }
    if (ImGui::Button("Load brainstem")) {
      std::vector<util::Uuid> animIds;
      loadGLTF(
        _scene,
        std::string(ASSET_PATH) + "models/brainstem_gltf/BrainStem.glb",
        _brainstemModelId,
        _brainstemMaterials,
        _brainstemSkelId,
        animIds);
      _brainstemAnimId = animIds[0];
    }
    if (ImGui::Button("Load fox")) {
      loadGLTF(
        _scene,
        std::string(ASSET_PATH) + "models/fox_gltf/fox_indexed.gltf",
        _foxModelId,
        _foxMaterials,
        _foxSkeleId,
        _foxAnims);
    }
    if (ImGui::Button("Remove random renderable")) {
      if (!_rends.empty()) {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<> idxRandomiser(0, (int)(_rends.size() - 1));

        auto idx = idxRandomiser(rng);

        auto id = _rends[idx];
        removeRenderable(_vkRenderer, id);

        _rends.erase(_rends.begin() + idx);
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

      rend._boundingSphere = glm::vec4(0.0f, 0.0f, 0.0f, 20.0f);
      rend._visible = true;
      rend._transform = glm::translate(glm::mat4(1.0f), worldPos);

      _scene.addRenderable(std::move(rend));
    });
}

util::Uuid& AneditApplication::selectedRenderable()
{
  return _selectedRenderable;
}

render::Camera& AneditApplication::camera()
{
  return _camera;
}
