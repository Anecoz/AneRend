#pragma once

#include "Application.h"

#include "AneditContext.h"
#include "../gui/IGUI.h"

#include <glm/glm.hpp>

#include "../render/VulkanRenderer.h"
#include "../render/asset/Model.h"
#include "../render/Camera.h"
#include "../render/RenderDebugOptions.h"
#include "../render/RenderOptions.h"
#include "../render/asset/Model.h"
#include "../render/asset/Renderable.h"
#include "../render/scene/Scene.h"
#include "../render/scene/ScenePager.h"
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

  render::RenderableId& selectedRenderable() override final;

  render::Camera& camera() override final;

  // AneditContext end

private:
  void setupGuis();
  void oldUI();
  void calculateShadowMatrix();

  std::vector<gui::IGUI*> _guis;

  glm::vec3 _lastCamPos;

  glm::vec3 _sunDir;

  render::RenderDebugOptions _renderDebugOptions;
  render::RenderOptions _renderOptions;
  logic::WindSystem _windSystem;
  glm::vec2 _windDir;

  render::Camera _camera;
  render::Camera _shadowCamera;

  render::VulkanRenderer _vkRenderer;

  render::ModelId _lanternModelId;
  std::vector<render::MaterialId> _lanternMaterials;

  render::ModelId _sponzaModelId;
  std::vector<render::MaterialId> _sponzaMaterialIds;

  render::ModelId _brainstemModelId;
  std::vector<render::MaterialId> _brainstemMaterials;
  render::AnimationId _brainstemAnimId;
  render::SkeletonId _brainstemSkelId;

  render::ModelId _shrekModelId;
  std::vector<render::MaterialId> _shrekMaterials;
  render::AnimationId _shrekAnimId;
  render::SkeletonId _shrekSkeleId;

  render::ModelId _foxModelId;
  std::vector<render::MaterialId> _foxMaterials;
  std::vector<render::AnimationId> _foxAnims;
  render::SkeletonId _foxSkeleId;
  render::asset::Renderable _cachedFoxRenderable;
  render::asset::Animator _foxAnimator;
  int _foxAnimIdx = 0;

  std::vector<render::RenderableId> _rends;

  std::vector<render::AnimationId> _dummyAnimations;
  render::SkeletonId _dummySkele;

  render::scene::Scene _scene;
  std::future<render::scene::DeserialisedSceneData> _sceneFut;
  render::scene::ScenePager _scenePager;
  std::filesystem::path _scenePath;
};