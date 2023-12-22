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
#include <render/asset/Renderable.h>
#include <render/scene/Scene.h>
#include <render/scene/ScenePager.h>
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

  std::vector<util::Uuid>& selection() override final;

  render::Camera& camera() override final;

  // AneditContext end

private:
  void setupGuis();
  void updateConfig();
  void addGltfDataToScene(std::unique_ptr<logic::LoadedGLTFData> data);
  void oldUI();
  void calculateShadowMatrix();
  util::Uuid instantiate(const render::asset::Prefab& prefab, util::Uuid parentRendUuid, glm::mat4 parentGlobalTransform);

  std::vector<gui::IGUI*> _guis;

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

  render::VulkanRenderer _vkRenderer;

  util::Uuid _lanternModelId;
  std::vector<util::Uuid> _lanternMaterials;

  util::Uuid _sponzaModelId;
  std::vector<util::Uuid> _sponzaMaterialIds;

  util::Uuid _brainstemModelId;
  std::vector<util::Uuid> _brainstemMaterials;
  util::Uuid _brainstemAnimId;
  util::Uuid _brainstemSkelId;

  util::Uuid _shrekModelId;
  std::vector<util::Uuid> _shrekMaterials;
  util::Uuid _shrekAnimId;
  util::Uuid _shrekSkeleId;

  util::Uuid _foxModelId;
  std::vector<util::Uuid> _foxMaterials;
  std::vector<util::Uuid> _foxAnims;
  util::Uuid _foxSkeleId;
  render::asset::Renderable _cachedFoxRenderable;
  render::asset::Animator _foxAnimator;
  int _foxAnimIdx = 0;

  std::vector<util::Uuid> _rends;

  std::vector<util::Uuid> _dummyAnimations;
  util::Uuid _dummySkele;

  render::scene::Scene _scene;
  std::future<render::scene::DeserialisedSceneData> _sceneFut;
  render::scene::ScenePager _scenePager;

  // Test bake
  bool _baking = false;
};