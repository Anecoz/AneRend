#version 450

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "scene_ubo.glsl"
#include "bindless.glsl"

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragTint;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;
layout(location = 4) in vec2 fragUV;
layout(location = 5) in flat uint fragMaterialIdx;
layout(location = 6) in vec3 fragTangent;
layout(location = 7) in mat3 fragTBN;

layout(location = 0) out vec4 outCol0;
layout(location = 1) out vec4 outCol1;
layout(location = 2) out vec4 outCol2;

void main() {
  vec3 normal = normalize(fragNormal);

  uint matIndex = rendMatIndexBuffer.indices[fragMaterialIdx];
  MaterialInfo matInfo = materialBuffer.infos[matIndex];

  bool discardPixel;
  SurfaceData surfData = getSurfaceDataFromMat(matInfo, fragUV, normal, fragTBN, fragTangent, fragColor, discardPixel);

  if (discardPixel) {
    discard;
  }

  vec3 color = surfData.color;
  if (length(fragTint) > 0.01) {
    color *= fragTint;
  }

  outCol0 = vec4(surfData.normal, color.x);
  outCol1 = vec4(color.yz, surfData.metallic, surfData.roughness);
  outCol2 = vec4(surfData.emissive, 0.0);
}