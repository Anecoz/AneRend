#pragma once

#include "Application.h"

#include <glm/glm.hpp>

#include "../render/VulkanRenderer.h"
#include "../render/Model.h"
#include "../render/Camera.h"
#include "../render/RenderDebugOptions.h"
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

  render::VulkanRenderer _vkRenderer;
  render::RenderDebugOptions _renderDebugOptions;
  logic::WindSystem _windSystem;

  render::Camera _camera;
  render::Camera _shadowCamera;

  render::Model _testModel;
  render::Model _testModel2;
  render::RenderableId _modelId;
  render::RenderableId _modelId2;
  render::MeshId _meshId;
  render::MeshId _meshId2;
};