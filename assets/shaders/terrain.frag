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
layout(location = 5) in vec3 fragTangent;
layout(location = 6) in float fragTangentSign;
layout(location = 7) in flat int fragTerrainIdx;
layout(location = 8) in flat uint fragMaterialIdx0;
layout(location = 9) in flat uint fragMaterialIdx1;
layout(location = 10) in flat uint fragMaterialIdx2;
layout(location = 11) in flat uint fragMaterialIdx3;

layout(location = 0) out vec4 outCol0;
layout(location = 1) out vec4 outCol1;
layout(location = 2) out vec4 outCol2;

void main() {
  vec3 normal = normalize(fragNormal);
  vec3 tangent = vec3(0.0);
  vec3 B = vec3(0.0);
  mat3 TBN = mat3(0.0);
  if (fragTangent != vec3(0.0)) {
    tangent = normalize(fragTangent);
    B = normalize(cross(normal, tangent) * fragTangentSign);
    TBN = mat3(tangent, B, normal);
  }

  // Find the terrain-wide UV
  int wTiX = int(fragPos.x) / int(TILE_SIZE);
  int wTiZ = int(fragPos.z) / int(TILE_SIZE);

  // Calc normalized tile coordinate
  float u = (fragPos.x - float(TILE_SIZE) * float(wTiX)) / float(TILE_SIZE);
  float v = (fragPos.z - float(TILE_SIZE) * float(wTiZ)) / float(TILE_SIZE);
  int blendMapIdx = terrainInfoBuffer.terrains[fragTerrainIdx].texIndices[0];
  vec4 blendSample = texture(textures[nonuniformEXT(blendMapIdx)], vec2(u, v));

  // Default values
  vec3 color = vec3(1.0);
  float roughness = 0.8;
  float metallic = 0.0;
  vec3 emissive = vec3(0.0);

  // Blend according to blendSample between the provided materials
  ivec4 matIndices = terrainInfoBuffer.terrains[fragTerrainIdx].baseMaterials;

  bool discardPixel;
  if (matIndices[0] != -1) {
    MaterialInfo matInfo = materialBuffer.infos[matIndices[0]];
    SurfaceData surfData = getSurfaceDataFromMat(matInfo, fragUV, normal, TBN, tangent, fragColor, discardPixel);

    color = blendSample[0] * surfData.color;
    roughness = blendSample[0] * surfData.roughness;
    metallic = blendSample[0] * surfData.metallic;
    emissive = blendSample[0] * surfData.emissive;
    normal = blendSample[0] * surfData.normal;
  }
  if (matIndices[1] != -1) {
    MaterialInfo matInfo = materialBuffer.infos[matIndices[1]];
    SurfaceData surfData = getSurfaceDataFromMat(matInfo, fragUV, normal, TBN, tangent, fragColor, discardPixel);

    color += blendSample[1] * surfData.color;
    roughness += blendSample[1] * surfData.roughness;
    metallic += blendSample[1] * surfData.metallic;
    emissive += blendSample[1] * surfData.emissive;
    normal += blendSample[1] * surfData.normal;
  }
  if (matIndices[2] != -1) {
    MaterialInfo matInfo = materialBuffer.infos[matIndices[2]];
    SurfaceData surfData = getSurfaceDataFromMat(matInfo, fragUV, normal, TBN, tangent, fragColor, discardPixel);

    color += blendSample[2] * surfData.color;
    roughness += blendSample[2] * surfData.roughness;
    metallic += blendSample[2] * surfData.metallic;
    emissive += blendSample[2] * surfData.emissive;
    normal += blendSample[2] * surfData.normal;
  }
  if (matIndices[3] != -1) {
    MaterialInfo matInfo = materialBuffer.infos[matIndices[3]];
    SurfaceData surfData = getSurfaceDataFromMat(matInfo, fragUV, normal, TBN, tangent, fragColor, discardPixel);

    color += blendSample[3] * surfData.color;
    roughness += blendSample[3] * surfData.roughness;
    metallic += blendSample[3] * surfData.metallic;
    emissive += blendSample[3] * surfData.emissive;
    normal += blendSample[3] * surfData.normal;
  }

  //color = blendSample.xyz;

  outCol0 = vec4(normal, color.x);
  outCol1 = vec4(color.yz, metallic, roughness);
  outCol2 = vec4(emissive, 0.0);
}