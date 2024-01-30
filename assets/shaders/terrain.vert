#version 450

#extension GL_GOOGLE_include_directive : enable
#include "scene_ubo.glsl"
#include "bindless.glsl"

struct TranslationId
{
  uint renderableIndex;
  uint meshOffset;
};

layout(std430, set = 1, binding = 0) buffer TranslationBuffer {
  TranslationId ids[];
} translationBuffer;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec4 inTangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragTint;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPos;
layout(location = 4) out vec2 fragUV;
layout(location = 5) out vec3 fragTangent;
layout(location = 6) out float fragTangentSign;
layout(location = 7) out flat int fragTerrainIdx;
layout(location = 8) out flat uint fragMaterialIdx0;
layout(location = 9) out flat uint fragMaterialIdx1;
layout(location = 10) out flat uint fragMaterialIdx2;
layout(location = 11) out flat uint fragMaterialIdx3;

void main() {
  uint renderableIndex = translationBuffer.ids[gl_InstanceIndex].renderableIndex;
  mat4 model = renderableBuffer.renderables[renderableIndex].transform;

  vec3 pos = inPosition;
  vec3 normal = inNormal;

  gl_Position = ubo.proj * ubo.view * model * vec4(pos, 1.0);
  fragColor = toLinear(vec4(inColor, 1.0)).rgb;
  fragTint = renderableBuffer.renderables[renderableIndex].tint.rgb;
  fragNormal =  normalize(mat3(model) * normal);
  fragPos = (model * vec4(pos, 1.0)).xyz;
  fragUV = inUV;

  //uint meshOffset = translationBuffer.ids[gl_InstanceIndex].meshOffset;
  //uint firstMeshOffset = renderableBuffer.renderables[renderableIndex].modelOffset;
  //uint materialOffset = meshOffset - firstMeshOffset;
  //fragMaterialIdx = renderableBuffer.renderables[renderableIndex].firstMaterialIndex + materialOffset;

  fragTerrainIdx = renderableBuffer.renderables[renderableIndex].terrainOffset;

  fragMaterialIdx0 = terrainInfoBuffer.terrains[fragTerrainIdx].baseMaterials[0];
  fragMaterialIdx1 = terrainInfoBuffer.terrains[fragTerrainIdx].baseMaterials[1];
  fragMaterialIdx2 = terrainInfoBuffer.terrains[fragTerrainIdx].baseMaterials[2];
  fragMaterialIdx3 = terrainInfoBuffer.terrains[fragTerrainIdx].baseMaterials[3];

  fragTangent = mat3(model) * inTangent.xyz;
  fragTangentSign = inTangent.w;
}