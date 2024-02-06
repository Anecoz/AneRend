#pragma once

#include "Application.h"

#include "AneditContext.h"
#include "AneditConfig.h"
#include "GLTFImporter.h"
#include "../gui/IGUI.h"

#include <glm/glm.hpp>

#include <render/VulkanRenderer.h>
#include <render/asset/Model.h>
#include <render/Camera.h>
#include <render/RenderDebugOptions.h>
#include <render/RenderOptions.h>
#include <render/asset/Model.h>
#include <render/scene/Scene.h>
#include <render/scene/ScenePager.h>
#include <render/cinematic/CinematicPlayer.h>
#include <render/animation/AnimationUpdater.h>
#include <physics/PhysicsSystem.h>
#include <terrain/TerrainSystem.h>
#include "WindSystem.h"

#include <filesystem>

class AneditApplication : public Application, public logic::AneditContext
{
public:
  AneditApplication(std::string title);
  ~AneditApplication();

  bool init();

  void render() override final;
  void update(double delta) override final;

  void notifyFramebufferResized();

  // AneditContext begin

  render::scene::Scene& scene() override final;
  void serializeScene() override final;
  void loadSceneFrom(std::filesystem::path p) override final;
  void setScenePath(std::filesystem::path p) override final;

  void startLoadGLTF(std::filesystem::path p) override final;

  void spawnFromPrefabAtMouse(const util::Uuid& prefab) override final;

  // Does not take children/parents into account!
  render::asset::Prefab prefabFromNode(const util::Uuid& node) override final;

  void* getImguiTexId(util::Uuid& tex) override final;
  void generateMipMaps(render::asset::Texture& tex) override final;

  void createCinematicPlayer(util::Uuid& id) override final;
  void destroyCinematicPlayer(util::Uuid& id) override final;
  void playCinematic(util::Uuid& id) override final;
  void pauseCinematic(util::Uuid& id) override final;
  void stopCinematic(util::Uuid& id) override final;
  double getCinematicTime(util::Uuid& id) override final;
  void setCinematicTime(util::Uuid& id, double time) override final;

  std::vector<util::Uuid>& selection() override final;

  render::Camera& camera() override final;

  virtual glm::vec3 latestWorldPosition() override final { return _latestWorldPosition; }

  // AneditContext end

private:
  void setupGuis();
  void updateConfig();
  void addGltfDataToScene(std::unique_ptr<logic::LoadedGLTFData> data);
  void oldUI();
  void calculateShadowMatrix();
  // The map is <prefab, node>
  util::Uuid instantiate(const render::asset::Prefab& prefab, glm::mat4 parentGlobalTransform, std::unordered_map<util::Uuid, util::Uuid>& instantiatedNodes);
  void updateSkeletons(std::unordered_map<util::Uuid, util::Uuid>& prefabNodeMap);

  std::vector<gui::IGUI*> _guis;

  std::unordered_map<util::Uuid, render::cinematic::CinematicPlayer> _cinePlayers;

  std::filesystem::path _scenePath;
  logic::AneditConfig _config;

  logic::GLTFImporter _gltfImporter;

  glm::vec3 _lastCamPos;

  glm::vec3 _sunDir;

  render::RenderDebugOptions _renderDebugOptions;
  render::RenderOptions _renderOptions;
  logic::WindSystem _windSystem;
  glm::vec2 _windDir;

  render::Camera _camera;
  render::Camera _shadowCamera;

  render::scene::Scene _scene;
  render::VulkanRenderer _vkRenderer;
  std::future<render::scene::DeserialisedSceneData> _sceneFut;
  render::scene::ScenePager _scenePager;

  render::anim::AnimationUpdater _animUpdater;
  terrain::TerrainSystem _terrainSystem;
  physics::PhysicsSystem _physicsSystem;

  glm::vec3 _latestWorldPosition = glm::vec3(0.0f);

  // Test bake
  bool _baking = false;

  // Test debug lines

};