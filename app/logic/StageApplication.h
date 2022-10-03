#pragma once

#include "Application.h"

#include "../render/VulkanRenderer.h"
#include "../render/Model.h"
#include "../render/Camera.h"

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
  render::VulkanRenderer _vkRenderer;

  render::Camera _camera;

  render::Model _testModel;
  render::Model _testModel2;
  render::RenderableId _modelId;
  render::RenderableId _modelId2;
};