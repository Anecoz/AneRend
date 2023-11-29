#pragma once

#include "../../util/Uuid.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace render::asset {

struct Light
{
  util::Uuid _id = util::Uuid::generate();

  std::string _name;

  glm::vec3 _pos = glm::vec3(0.0f);
  glm::vec3 _color = glm::vec3(1.0f, 0.0f, 0.0f);
  float _range = 2.0f;
  bool _enabled = true;
  bool _shadowCaster = true;

  // Shadow caster stuff
  glm::mat4 _proj;
  std::vector<glm::mat4> _views;

  void updateViewMatrices()
  {
    // Also do projection matrix here...
    _proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, _range);

    _views.clear();
    // Construct view matrices
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), _pos);
    glm::mat4 view(1.0f);
    glm::mat4 rot(1.0f);
    glm::mat4 scale(1.0f);

    // Front
    rot = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    scale = glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
    view = glm::inverse(trans * rot * scale);
    _views.emplace_back(view);

    // Back
    rot = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    scale = glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
    view = glm::inverse(trans * rot * scale);
    _views.emplace_back(view);

    // Up
    rot = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    auto rotYD = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    scale = glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
    view = glm::inverse(trans * rotYD * rot * scale);
    _views.emplace_back(view);

    // Down
    rot = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    auto rotYU = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    scale = glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
    view = glm::inverse(trans * rotYU * rot * scale);
    _views.emplace_back(view);

    // Right
    rot = glm::rotate(glm::mat4(1.0f), glm::radians(-180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    scale = glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
    view = glm::inverse(trans * rot * scale);
    _views.emplace_back(view);

    // Left
    rot = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    scale = glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f));
    view = glm::inverse(trans * rot * scale);
    _views.emplace_back(view);
  }

  // TODO:
  // Type, i.e. directional, point, spot
};

}