#pragma once 

#include "../Identifiers.h"

#include <glm/glm.hpp>

#include <vector>

namespace render::asset {

struct Renderable
{
  render::RenderableId _id = INVALID_ID;
  render::ModelId _model;
  render::AnimationId _animation = INVALID_ID;
  render::SkeletonId _skeleton = INVALID_ID;
  std::vector<render::MaterialId> _materials; // one for each mesh in the model (could be same ID for multiple meshes tho)

  glm::mat4 _transform;
  glm::vec3 _tint;
  glm::vec4 _boundingSphere; // xyz sphere center, w radius
  bool _visible = true;
};

}