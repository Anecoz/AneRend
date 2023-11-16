#version 450

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "scene_ubo.glsl"
#include "bindless.glsl"

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec2 fragUV;
layout(location = 4) in flat uint fragMaterialIdx;
layout(location = 5) in vec3 fragTangent;
layout(location = 6) in mat3 fragTBN;

layout(location = 0) out vec4 outCol0;
layout(location = 1) out vec4 outCol1;
layout(location = 2) out vec4 outCol2;

void main() {
  vec3 normal = normalize(fragNormal);

  uint matIndex = rendMatIndexBuffer.indices[fragMaterialIdx];
  MaterialInfo matInfo = materialBuffer.infos[matIndex];

  SurfaceData surfData = getSurfaceDataFromMat(matInfo, fragUV, normal, fragTBN, fragTangent, fragColor);

  outCol0 = vec4(surfData.normal, surfData.color.x);
  outCol1 = vec4(surfData.color.yz, surfData.metallic, surfData.roughness);
  outCol2 = vec4(surfData.emissive, 0.0);

  /*outCol0 = vec4(fragNormal, fragColor.x);
  outCol1 = vec4(fragColor.yz, 0.5, 0.5);
  outCol2 = vec4(0.0);*/
}