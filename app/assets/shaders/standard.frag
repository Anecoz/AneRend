#version 450

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "scene_ubo.glsl"
#include "bindless.glsl"

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec2 fragUV;
layout(location = 4) in flat uint fragMeshId;
layout(location = 5) in vec3 fragTangent;
layout(location = 6) in mat3 fragTBN;

layout(location = 0) out vec4 outCol0;
layout(location = 1) out vec4 outCol1;
//layout(location = 2) out vec4 outCol2;

void main() {
  vec3 normal = normalize(fragNormal);
  float metallic = 0.1;
  float roughness = 0.8;
  vec3 color = fragColor;

  MeshMaterialInfo matInfo = materialBuffer.infos[fragMeshId];

  if (matInfo.metallicTexIndex != -1) {
    metallic = texture(textures[nonuniformEXT(matInfo.metallicTexIndex)], fragUV).r;
  }
  if (matInfo.roughnessTexIndex != -1) {
    roughness = texture(textures[nonuniformEXT(matInfo.roughnessTexIndex)], fragUV).r;
  }
  if (matInfo.albedoTexIndex != -1) {
    vec4 samp = texture(textures[nonuniformEXT(matInfo.albedoTexIndex)], fragUV);
    if (samp.a < 0.1) {
      discard;
    }
    color = samp.rgb;
  }
  if (matInfo.metallicRoughnessTexIndex != -1) {
    vec4 samp = texture(textures[nonuniformEXT(matInfo.metallicRoughnessTexIndex)], fragUV);
    metallic = samp.b;
    roughness = samp.g;
  }
  if (matInfo.normalTexIndex != -1 && fragTangent.xyz != vec3(0.0f)) {
    normal = normalize(texture(textures[nonuniformEXT(matInfo.normalTexIndex)], fragUV).rgb * 2.0 - 1.0);
    normal = normalize(fragTBN * normal);
  }

  outCol0 = vec4(normal * 0.5 + 0.5, color.x);
  outCol1 = vec4(color.yz, metallic, roughness);
  //outCol2 = vec4(fragPos, 0.0);
}