#pragma once

//#include "../ImageHelpers.h"
#include "../../util/Uuid.h"

#include <string>

#include <glm/glm.hpp>

namespace render::asset {

struct Material
{
  util::Uuid _id = util::Uuid::generate();

  std::string _name;

  // This is either a factor applied to the albedo texture, or (if no albedo present) treated as linear RGB values.
  glm::vec3 _baseColFactor = glm::vec3(1.0f);

  // RGB is the color and W is the strength. If an emissive texture is present, the RGB are treated as multipliers.
  glm::vec4 _emissive = glm::vec4(0.0f);

  float _metallicFactor = 1.0f;
  float _roughnessFactor = 1.0f;

  // G-channel roughness, B-channel metallic.
  util::Uuid _metallicRoughnessTex;

  util::Uuid _albedoTex;

  // In tangent space, only really usable if tangents are present as vertex attributes.
  util::Uuid _normalTex;

  // See _emissive member above.
  util::Uuid _emissiveTex;
};

}