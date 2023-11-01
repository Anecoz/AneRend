#pragma once

#include "Application.h"

#include <glm/glm.hpp>

#include "../render/VulkanRenderer.h"
#include "../render/asset/Model.h"
#include "../render/Camera.h"
#include "../render/RenderDebugOptions.h"
#include "../render/RenderOptions.h"
#include "../render/asset/Model.h"
#include "../render/asset/Renderable.h"
#include "WindSystem.h"

class StageApplication : public Application
{
public:
  StageApplication(std::string title);
  ~StageApplication();

  bool init();

  void render() override final;
  void update(double delta) override final;

  void notifyFramebufferResized();

private:
  void calculateShadowMatrix();

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

  render::asset::Model _sponzaModel;
  std::vector<render::asset::Material> _sponzaMaterials;
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
  int _foxAnim = 0;

  std::vector<render::RenderableId> _rends;

  std::vector<render::AnimationId> _dummyAnimations;
  render::SkeletonId _dummySkele;
};