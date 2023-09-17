#pragma once

#include "Application.h"

#include <glm/glm.hpp>

#include "../render/VulkanRenderer.h"
#include "../render/Model.h"
#include "../render/Camera.h"
#include "../render/RenderDebugOptions.h"
#include "../render/RenderOptions.h"
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

  render::RenderableId _sponzaId;
  glm::mat4 _sponzaScale;
  glm::vec3 _sponzaPos;
  bool _sponzaMoved = false;

  glm::vec3 _sunDir;

  render::RenderDebugOptions _renderDebugOptions;
  render::RenderOptions _renderOptions;
  logic::WindSystem _windSystem;
  glm::vec2 _windDir;

  render::Camera _camera;
  render::Camera _shadowCamera;

  render::VulkanRenderer _vkRenderer;

  render::ModelId _meshId;
  render::ModelId _meshId2;
  render::ModelId _meshId3;
  render::ModelId _meshId4;
  render::ModelId _meshId5;
  render::ModelId _meshId6;
  render::ModelId _meshId7;
  render::ModelId _meshId8;
  render::ModelId _meshId9;
};