#pragma once

#include "../util/Uuid.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <optional>
#include <string>
#include <vector>

namespace component {

/*
* If you add a new component, update the PotentialComponent struct below aswell!
*/

struct Transform
{
  glm::mat4 _globalTransform;
  glm::mat4 _localTransform;
};

struct Renderable
{
  //util::Uuid _id = util::Uuid::generate();

  std::string _name; // for debugging

  util::Uuid _model;
  util::Uuid _skeleton;
  std::vector<util::Uuid> _materials; // one for each mesh in the model (could be same ID for multiple meshes tho)

  glm::vec3 _tint;
  glm::vec4 _boundingSphere; // xyz sphere center, w radius
  bool _visible = true;
};

struct PageStatus
{
  bool _paged = true;
};

struct Light
{
  //util::Uuid _id = util::Uuid::generate();

  std::string _name;

  glm::vec3 _color = glm::vec3(1.0f, 0.0f, 0.0f);
  float _range = 5.0f;
  bool _enabled = true;
  bool _shadowCaster = true;

  // Shadow caster stuff
  glm::mat4 _proj;
  std::vector<glm::mat4> _views;

  void updateViewMatrices(const glm::vec3& pos)
  {
    // Also do projection matrix here...
    _proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, _range);

    _views.clear();
    // Construct view matrices
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), pos);
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

// Struct that holds potential components used by e.g. prefabs
struct PotentialComponents
{
  Transform _trans; // not optional
  std::optional<Renderable> _rend;
  std::optional<Light> _light;
};

}