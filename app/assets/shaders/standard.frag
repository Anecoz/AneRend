#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 invProj;
  mat4 invViewProj;
  mat4 directionalShadowMatrixProj;
  mat4 directionalShadowMatrixView; 
  mat4 shadowMatrix[24];
  vec4 cameraPos;
  vec4 lightDir;
  vec4 viewVector;
  float time;
  uint screenWidth;
  uint screenHeight;
} ubo;

struct MeshMaterialInfo
{
  int metallicTexIndex;
  int roughnessTexIndex;
  int normalTexIndex;
  int albedoTexIndex;
  int metallicRoughnessTexIndex;
};

layout(std430, set = 0, binding = 5) readonly buffer MeshMaterialBuffer {
  MeshMaterialInfo infos[];
} materialBuffer;

layout(set = 0, binding = 6) uniform sampler2D textures[];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec2 fragUV;
layout(location = 4) in flat uint fragMeshId;

layout(location = 0) out vec4 outCol0;
layout(location = 1) out vec4 outCol1;
//layout(location = 2) out vec4 outCol2;

void main() {
  vec3 normal = normalize(fragNormal);
  float metallic = 0.3;
  float roughness = 0.5;
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
  /*if (fragNormalIndex != -1) {
    normal = normalize(fragModelMtx * (texture(textures[nonuniformEXT(fragNormalIndex)], uv).rgb * 2.0 - 1.0));
  }*/

  outCol0 = vec4(normal, color.x);
  outCol1 = vec4(color.yz, metallic, roughness);
  //outCol2 = vec4(fragPos, 0.0);
}