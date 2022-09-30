#pragma once

#include "Application.h"

#include "../render/VulkanRenderer.h"

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
  VulkanRenderer _vkRenderer;
};