#pragma once

#include "../Identifiers.h"
#include "../ImageHelpers.h"

#include <glm/glm.hpp>

namespace render::asset {

struct Material
{
  MaterialId _id = INVALID_ID;

  // This is either a factor applied to the albedo texture, or (if no albedo present) treated as linear RGB values.
  glm::vec3 _baseColFactor = glm::vec3(1.0f);

  // RGB is the color and W is the strength. If an emissive texture is present, the RGB are treated as multipliers.
  glm::vec4 _emissive = glm::vec4(0.0f);

  // G-channel roughness, B-channel metallic.
  imageutil::TextureData _metallicRoughnessTex;

  imageutil::TextureData _albedoTex;

  // In tangent space, only really usable if tangents are present as vertex attributes.
  imageutil::TextureData _normalTex;

  // See _emissive member above.
  imageutil::TextureData _emissiveTex;
};

}