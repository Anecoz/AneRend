#include "AneditApplication.h"

#include "../../common/input/KeyInput.h"
#include "../../common/input/MousePosInput.h"
#include <util/GLTFLoader.h>
#include <util/TextureHelpers.h>
#include <render/ImageHelpers.h>
#include <render/cinematic/CinematicPlayer.h>

#include "../gui/SceneAssetGUI.h"
#include "../gui/SceneListGUI.h"
#include "../gui/EditSelectionGUI.h"

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
  , _windDir(glm::vec2(-1.0, 0.0))
  , _shadowCamera(glm::vec3(-20.0f, 15.0f, -20.0f), render::ProjectionType::Orthogonal)
  , _scene()
  , _vkRenderer(_window, _camera, &_scene.registry())
  , _scenePager(&_vkRenderer)
  , _animUpdater(&_scene)
  , _terrainSystem(&_scene)
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
      _animUpdater.setScene(&_scene);
      _terrainSystem.setScene(&_scene);
      _vkRenderer.setRegistry(&_scene.registry());
      scenePtr = nullptr;
    }
  }

  // Check if we have a GLTF model that is being loaded
  if (_gltfImporter.hasResult()) {
    auto gltfData = _gltfImporter.takeResult();
    addGltfDataToScene(std::move(gltfData));
  }

  _camera.update(delta);
  {
    auto now = std::chrono::system_clock::now();
    _animUpdater.update(delta);
    auto after = std::chrono::system_clock::now();

    //printf("Anim took %lld us\n", std::chrono::duration_cast<std::chrono::microseconds>(after - now).count());
  }

  _terrainSystem.update();

  {
    auto now = std::chrono::system_clock::now();
    _scene.update();
    auto after = std::chrono::system_clock::now();

    //printf("Scene update took %lld us\n", std::chrono::duration_cast<std::chrono::microseconds>(after - now).count());
  }

  _scenePager.update(_camera.getPosition());
  _scene.resetEvents();

  for (auto it = _cinePlayers.begin(); it != _cinePlayers.end(); ++it) {
    it->second.update(delta);
  }

  //_windSystem.update(delta);
  _windSystem.setWindDir(glm::normalize(_windDir));

  // For now always request a world pos
  _vkRenderer.requestWorldPosition(MousePosInput::getPosition(),
    [this](glm::vec3 worldPos) {
      _latestWorldPosition = worldPos;
    });

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
    auto ypr = _camera.getYPR();

    ImGui::Begin("Debug");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("Camera pos %.1f, %.1f, %.1f", _camera.getPosition().x, _camera.getPosition().y, _camera.getPosition().z);
    ImGui::Text("Camera ypr %.1f, %.1f, %.1f", ypr.x, ypr.y, ypr.z);
    if (ImGui::Button("Teleport to origin")) {
      _camera.setPosition({ 0.0f, 0.0f, 0.0f });
    }
    ImGui::End();
  }

  for (auto gui : _guis) {
    gui->immediateDraw(this);
  }

  oldUI();

  {
    auto now = std::chrono::system_clock::now();
    _vkRenderer.drawFrame();
    auto after = std::chrono::system_clock::now();

    /*static int counter = 0;
    if (counter++ % 50) {
      printf("Recording frame took %lld us\n", std::chrono::duration_cast<std::chrono::microseconds>(after - now).count());
    }*/

  }

}

void AneditApplication::setupGuis()
{
  _guis.emplace_back(new gui::SceneAssetGUI());
  _guis.emplace_back(new gui::SceneListGUI());
  _guis.emplace_back(new gui::EditSelectionGUI());
}

void AneditApplication::updateConfig()
{
  _config._scenePath = _scenePath;
}

void AneditApplication::addGltfDataToScene(std::unique_ptr<logic::LoadedGLTFData> data)
{
  for (auto& tex : data->_textures) {
    // Generate mips for all textures using renderContext (unless they specifically ask not to maybe?)
    _vkRenderer.generateMipMaps(tex);

    // And then compress all mips here using TextureHelpers
    if (render::imageutil::numDimensions(tex._format) >= 3) {
      util::TextureHelpers::convertRGBA8ToBC7(tex);
    }
    else if (render::imageutil::numDimensions(tex._format) == 2) {
      util::TextureHelpers::convertRG8ToBC5(tex);
    }
  }

  // TEMP
  auto prefabsCopy = data->_prefabs;
  
  for (auto& model : data->_models) {
    _scene.addModel(std::move(model));
  }

  for (auto& prefab : data->_prefabs) {
    _scene.addPrefab(std::move(prefab));
  }

  /*for (auto& skeleton : data->_skeletons) {
    _scene.addSkeleton(std::move(skeleton));
  }*/

  for (auto& anim : data->_animations) {
    _scene.addAnimation(std::move(anim));
  }

  for (auto& tex : data->_textures) {
    _scene.addTexture(std::move(tex));
  }

  for (auto& mat : data->_materials) {
    _scene.addMaterial(std::move(mat));
  }

  // TESTING: Instantiate _everything_
  for (auto& prefab : prefabsCopy) {
    // Only instantiate if no parent
    if (!prefab._parent) {
      glm::mat4 mtx(1.0f);
      for (std::size_t x = 0; x < 1; ++x) {
        for (std::size_t y = 0; y < 1; ++y) {
          mtx = glm::translate(glm::mat4(1.0f), glm::vec3((float)(x + 1), 0.0f, (float)(y + 1)));
          std::unordered_map<util::Uuid, util::Uuid> prefabNodeMap;
          instantiate(prefab, mtx, prefabNodeMap);
          updateSkeletons(prefabNodeMap);
        }
      }
    }
  }
}

void AneditApplication::oldUI()
{
  // Demo window is useful for exploring different ImGui functionality.
  //ImGui::ShowDemoWindow();

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
    ImGui::SliderFloat("Bloom threshold", &_renderOptions.bloomThresh, 0.1f, 20.0f);
    ImGui::SliderFloat("Bloom knee", &_renderOptions.bloomKnee, 0.0f, 1.0f);
    ImGui::ColorEdit3("Sky color", (float*) &_renderOptions.skyColor);
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

void AneditApplication::updateSkeletons(std::unordered_map<util::Uuid, util::Uuid>& prefabNodeMap)
{
  // TODO: Prefabs should contain a sack of nodes instead...
  for (auto& pair : prefabNodeMap) {
    auto& prefabId = pair.first;
    auto& nodeId = pair.second;
    if (_scene.registry().hasComponent<component::Skeleton>(nodeId)) {
      auto& skeleComp = _scene.registry().getComponent<component::Skeleton>(nodeId);
      
      for (auto& jr : skeleComp._jointRefs) {
        assert(prefabNodeMap.find(jr._node) != prefabNodeMap.end() && "Can't update skeleton, something really wrong!");

        jr._node = prefabNodeMap[jr._node];
      }
    }
  }
}

util::Uuid AneditApplication::instantiate(const render::asset::Prefab& prefab, glm::mat4 parentGlobalTransform, std::unordered_map<util::Uuid, util::Uuid>& instantiatedNodes)
{
  render::scene::Node node{};
  node._name = prefab._name;
  auto id = _scene.addNode(std::move(node));

  // There should always be a transform component
  auto globalTransform = parentGlobalTransform * prefab._comps._trans._localTransform;
  auto localTransform = prefab._comps._trans._localTransform;

  // Instantiate children
  auto& prefabs = _scene.getPrefabs();
  for (auto& childPrefabId : prefab._children) {
    for (auto& p : prefabs) {
      if (p._id == childPrefabId) {
        auto childId = instantiate(p, globalTransform, instantiatedNodes);
        _scene.addNodeChild(id, childId);
        break;
      }
    }
  }

  if (!prefab._parent) {
    localTransform = globalTransform;
  }

  _scene.registry().addComponent<component::Transform>(id, globalTransform, localTransform);
  _scene.registry().patchComponent<component::Transform>(id);

  if (prefab._comps._rend) {
    auto& prefabRend = prefab._comps._rend.value();
    auto& rendComp = _scene.registry().addComponent<component::Renderable>(id);
    rendComp = prefabRend;
    _scene.registry().patchComponent<component::Renderable>(id);
  }
  if (prefab._comps._light) {
    auto& prefabComp = prefab._comps._light.value();
    auto& comp = _scene.registry().addComponent<component::Light>(id);
    comp = prefabComp;
    _scene.registry().patchComponent<component::Light>(id);
  }
  if (prefab._comps._animator) {
    auto& prefabComp = prefab._comps._animator.value();
    auto& comp = _scene.registry().addComponent<component::Animator>(id);
    comp = prefabComp;
    _scene.registry().patchComponent<component::Animator>(id);
  }
  if (prefab._comps._skeleton) {
    auto& prefabComp = prefab._comps._skeleton.value();
    auto& comp = _scene.registry().addComponent<component::Skeleton>(id);
    comp = prefabComp;
    _scene.registry().patchComponent<component::Skeleton>(id);
  }

  instantiatedNodes[prefab._id] = id;

  return id;
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
      auto trans = glm::translate(glm::mat4(1.0f), worldPos);

      auto* p = _scene.getPrefab(prefab);
      std::unordered_map<util::Uuid, util::Uuid> prefabNodeMap;
      auto id = instantiate(*p, trans, prefabNodeMap);
      updateSkeletons(prefabNodeMap);

      // Also select it
      _selection.clear();
      _selection.emplace_back(id);
      _selectionType = AneditContext::SelectionType::Node;
    });
}

render::asset::Prefab AneditApplication::prefabFromNode(const util::Uuid& node)
{
  render::asset::Prefab p{};
  auto tmpId = p._id;
  p._id = util::Uuid();

  auto nodeP = _scene.getNode(node);
  if (!nodeP) {
    return p;
  }

  p._id = tmpId;
  p._name = nodeP->_name;

  // Set components
  auto id = node;
  p._comps = _scene.nodeToPotComps(id);

  return p;
}

void* AneditApplication::getImguiTexId(util::Uuid& tex)
{
  return _vkRenderer.getImGuiTexId(tex);
}

void AneditApplication::generateMipMaps(render::asset::Texture& tex)
{
  _vkRenderer.generateMipMaps(tex);
}

void AneditApplication::createCinematicPlayer(util::Uuid& id)
{
  if (_cinePlayers.find(id) == _cinePlayers.end()) {
    auto* cinematic = _scene.getCinematic(id);
    if (!cinematic) {
      printf("Cannot play cinematic %s, does not exist\n", id.str().c_str());
      return;
    }

    _cinePlayers[id] = render::cinematic::CinematicPlayer(cinematic, &_scene, &_camera);
  }
}

void AneditApplication::destroyCinematicPlayer(util::Uuid& id)
{
  if (_cinePlayers.find(id) != _cinePlayers.end()) {
    _cinePlayers.erase(id);
  }
}

void AneditApplication::playCinematic(util::Uuid& id)
{
  if (_cinePlayers.find(id) != _cinePlayers.end()) {
    _cinePlayers[id].play();
  }
}

void AneditApplication::pauseCinematic(util::Uuid& id)
{
  if (_cinePlayers.find(id) != _cinePlayers.end()) {
    _cinePlayers[id].pause();
  }
}

void AneditApplication::stopCinematic(util::Uuid& id)
{
  if (_cinePlayers.find(id) != _cinePlayers.end()) {
    _cinePlayers[id].stop();
  }
}

double AneditApplication::getCinematicTime(util::Uuid& id)
{
  if (_cinePlayers.find(id) != _cinePlayers.end()) {
    return _cinePlayers[id].getCurrentTime();
  }

  return 0.0;
}

void AneditApplication::setCinematicTime(util::Uuid& id, double time)
{
  if (_cinePlayers.find(id) != _cinePlayers.end()) {
    _cinePlayers[id].setCurrentTime(time);
  }
}

std::vector<util::Uuid>& AneditApplication::selection()
{
  return _selection;
}

render::Camera& AneditApplication::camera()
{
  return _camera;
}
