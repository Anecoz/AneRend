#pragma once

#include "AllocatedImage.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <vector>

namespace render {

// Either a point light or a directional light
// The order of a cubemap must be
// Front, back, up, down, right, left
struct Light
{
  enum Type {
    Directional,
    Point
  } _type;

  glm::vec3 _pos;
  glm::vec3 _color;
  bool _enabled = true;
  double _range;
  std::vector<glm::mat4> _view;
  glm::mat4 _proj;

  AllocatedImage _shadowImage; // Either layer count 1 or 6 depending on if directional or point
  std::vector<VkImageView> _shadowImageViews; // Either 1 or 6 if point. The 6 views make it easier to attach the layers of the cubemap as a depth attachment when rendering the shadowpass.
  VkImageView _cubeShadowView;
  VkSampler _sampler;

  double _debugCircleRadius;
  double _debugOrigX;
  double _debugOrigZ;
  double _debugSpeed;

  double _lastAngle = 0.0;

  void debugUpdatePos(double delta)
  {
    _lastAngle += delta * _debugSpeed;
    _pos.x = (float)_debugOrigX + (float)cos(_lastAngle) * (float)_debugCircleRadius;
    _pos.z = (float)_debugOrigZ + (float)sin(_lastAngle) * (float)_debugCircleRadius;
  }

  void updateViewMatrices()
  {
    _view.clear();
    // Construct view matrices
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), _pos);
    glm::mat4 view(1.0f);
    glm::mat4 rot(1.0f);
    glm::mat4 scale(1.0f);

    // Front
    rot = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    scale = glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
    view = glm::inverse(trans * rot * scale);
    _view.emplace_back(view);

    // Back
    rot = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    scale = glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
    view = glm::inverse(trans * rot * scale);
    _view.emplace_back(view);

    // Up
    rot = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    auto rotYU = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    scale = glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
    view = glm::inverse(trans * rotYU * rot * scale);
    _view.emplace_back(view);

    // Down
    rot = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    auto rotYD = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    scale = glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
    view = glm::inverse(trans * rotYD * rot * scale);
    _view.emplace_back(view);

    // Right
    rot = glm::rotate(glm::mat4(1.0f), glm::radians(-180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    scale = glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
    view = glm::inverse(trans * rot * scale);
    _view.emplace_back(view);

    // Left
    rot = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    scale = glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
    view = glm::inverse(trans * rot * scale);
    _view.emplace_back(view);
  }
};

}